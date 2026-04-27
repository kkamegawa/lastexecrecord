#include <Windows.h>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "CommandRunner.h"
#include "Config.h"
#include "FileUtil.h"
#include "InstallerUtil.h"
#include "Json.h"
#include "NetworkUtil.h"
#include "TimeUtil.h"

static constexpr DWORD kLockRetryIntervalMs = 1000;  // 1 second between file-lock retries
static constexpr DWORD kMaxSleepMs = (std::numeric_limits<DWORD>::max)(); // cap for Sleep()
static constexpr std::int64_t kDefaultLockTimeoutSeconds = 300;

static bool tryParseNonNegativeInt64(const std::wstring& text, std::int64_t& value) {
	if (text.empty()) return false;

	wchar_t* end = nullptr;
	errno = 0;
	long long parsed = std::wcstoll(text.c_str(), &end, 10);
	if (errno == ERANGE || end == text.c_str() || *end != L'\0' || parsed < 0) {
		return false;
	}

	value = static_cast<std::int64_t>(parsed);
	return true;
}

static void printUsage(const wchar_t* exeName) {
	std::wcout
		<< L"LastExecuteRecord - run commands from JSON config once per invocation\n\n"
		<< L"Copyright (c) 2026 Kazushi Kamegawa\n\n"
		<< L"Usage:\n"
		<< L"  " << exeName << L" [--config <path>] [--dry-run] [--verbose]\n\n"
		<< L"Options:\n"
		<< L"  --config <path>  Path to config JSON (default: %USERPROFILE%\\.lastexecrecord\\config.json)\n"
		<< L"  --dry-run        Do not execute; only show decisions\n"
		<< L"  --verbose        Print skip reasons and detailed output\n"
		<< L"  --lock-timeout <seconds>\n"
		<< L"                   Max time to wait for the config lock (default: 300)\n";
}

int wmain(int argc, wchar_t* argv[]) {
	bool dryRun = false;
	bool verbose = false;
	std::wstring configPath = ler::defaultConfigPath();
	std::int64_t lockTimeoutSeconds = kDefaultLockTimeoutSeconds;

	// Parse arguments (skip if argc <= 1, i.e., no arguments provided)
	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			std::wstring a = argv[i];
			if (a == L"--help" || a == L"-h" || a == L"/?") {
				printUsage(argv[0]);
				return 0;
			}
			if (a == L"--dry-run") {
				dryRun = true;
				continue;
			}
			if (a == L"--verbose") {
				verbose = true;
				continue;
			}
			if (a == L"--config") {
				if (i + 1 >= argc) {
					std::wcerr << L"--config requires a path\n";
					return 2;
				}
				configPath = argv[++i];
				continue;
			}
			if (a == L"--lock-timeout") {
				if (i + 1 >= argc) {
					std::wcerr << L"--lock-timeout requires seconds\n";
					return 2;
				}
				if (!tryParseNonNegativeInt64(argv[++i], lockTimeoutSeconds)) {
					std::wcerr << L"--lock-timeout must be a non-negative integer\n";
					return 2;
				}
				continue;
			}

			std::wcerr << L"Unknown argument: " << a << L"\n";
			printUsage(argv[0]);
			return 2;
		}
	}

	try {
		configPath = ler::getFullPath(configPath);
		std::wstring configDir = ler::getDirectoryName(configPath);
		if (configDir.empty()) {
			throw std::runtime_error("Config path has no directory component");
		}
		ler::ensureDirectoryExists(configDir);

		// Prevent concurrent runs against the same config file.
		// Retry until the lock is acquired or the bounded wait expires.
		std::wstring lockPath = configPath + L".lock";
		ler::FileLock lock = ler::tryAcquireLockFile(lockPath);
		if (lock.h == INVALID_HANDLE_VALUE) {
			std::wcout << L"[wait] Waiting for file lock: " << lockPath << L"\n";
			const std::int64_t maxTimeoutMs =
				static_cast<std::int64_t>((std::numeric_limits<DWORD>::max)());
			std::int64_t timeoutMs = lockTimeoutSeconds > maxTimeoutMs / 1000
				? maxTimeoutMs
				: lockTimeoutSeconds * 1000;
			std::int64_t waitedMs = 0;
			do {
				if (waitedMs >= timeoutMs) {
					std::wcerr << L"Fatal: timed out waiting for file lock after "
						<< lockTimeoutSeconds << L" seconds\n";
					return 2;
				}
				DWORD sleepMs = kLockRetryIntervalMs;
				std::int64_t remainingMs = timeoutMs - waitedMs;
				if (remainingMs < static_cast<std::int64_t>(sleepMs)) {
					sleepMs = static_cast<DWORD>(remainingMs);
				}
				Sleep(sleepMs);
				waitedMs += sleepMs;
				lock = ler::tryAcquireLockFile(lockPath);
			} while (lock.h == INVALID_HANDLE_VALUE);
		}

		// Auto-generate a sample config once while holding the same config lock.
		ler::ensureSampleConfigExists(configPath);

		ler::AppConfig cfg = ler::loadAndValidateConfig(configPath);

		// Check network status early if networkOption requires it
		if (!ler::shouldExecuteBasedOnNetwork(cfg.networkOption)) {
			if (verbose) {
				std::wcout << L"[skip] Skipping all commands due to network status (networkOption="
					<< static_cast<int>(cfg.networkOption) << L")\n";
			}
			return 0;
		}

		// If localOnly pinning updated config, persist it now.
		if (cfg.dirty) {
			ler::applyCommandsToJson(cfg);
			ler::writeWStringToUtf8FileAtomic(configPath, ler::writeJson(cfg.root));
			cfg.dirty = false;
		}

		std::int64_t now = ler::nowEpochSecondsUtc();
		int overallExit = 0;

		for (size_t idx = 0; idx < cfg.commands.size(); idx++) {
			ler::CommandConfig& c = cfg.commands[idx];

			if (!c.enabled) {
				if (verbose) std::wcout << L"[skip] " << c.name << L": disabled\n";
				continue;
			}

			bool haveLast = false;
			std::int64_t lastEpoch = 0;
			if (c.hasLastRunUtc) {
				if (ler::tryParseIsoUtcToEpochSeconds(c.lastRunUtc, lastEpoch)) {
					haveLast = true;
				}
				else {
					if (verbose) {
						std::wcout << L"[warn] " << c.name << L": lastRunUtc has invalid format; treating as never run\n";
					}
					c.hasLastRunUtc = false;
					cfg.dirty = true;
				}
			}

			if (haveLast && c.minIntervalSeconds > 0) {
				std::int64_t delta = now - lastEpoch;
				if (delta >= 0 && delta < c.minIntervalSeconds) {
					std::wcout << L"[skip] " << c.name << L": minIntervalSeconds not reached (" << delta
						<< L"/" << c.minIntervalSeconds << L" sec)\n";
					continue;
				}
			}

			if (dryRun) {
				std::wcout << L"[run ] " << c.name << L"\n";
				std::wcout << L"       exe: " << c.exe << L"\n";
				if (verbose && !c.args.empty()) {
					std::wcout << L"       args:";
					for (const auto& a : c.args) std::wcout << L" " << a;
					std::wcout << L"\n";
				}
				continue;
			}

			// Check for running installer processes
			if (ler::isInstallerRunning()) {
				if (c.installerWaitBehavior == ler::InstallerWaitBehavior::Skip) {
					std::wcout << L"[skip] " << c.name << L": installer process is running (configured to skip)\n";
					continue;
				}
				else {
					// Wait for installer to finish, printing a status line on each attempt.
					// Treat installerMaxRetries <= 0 as 1 (guarantee at least one wait attempt).
					std::int64_t maxRetries = c.installerMaxRetries > 0 ? c.installerMaxRetries : 1;

					// Compute the actual wait interval in seconds, matching the Sleep() duration.
					const std::int64_t maxSleepSeconds = static_cast<std::int64_t>(kMaxSleepMs) / 1000;
					std::int64_t effectiveWaitSeconds;
					if (c.installerWaitSeconds <= 0) {
						effectiveWaitSeconds = 1; // Ensure at least a minimal wait
					}
					else if (c.installerWaitSeconds > maxSleepSeconds) {
						effectiveWaitSeconds = maxSleepSeconds; // Cap to Sleep() limit
					}
					else {
						effectiveWaitSeconds = c.installerWaitSeconds;
					}
					DWORD sleepMs = static_cast<DWORD>(effectiveWaitSeconds * 1000);

					bool installerFinished = false;
					for (std::int64_t attempt = 0; attempt < maxRetries; ++attempt) {
						// Check immediately whether the installer has finished.
						if (!ler::isInstallerRunning()) {
							installerFinished = true;
							break;
						}

						// Only sleep if we have another attempt remaining.
						if (attempt + 1 < maxRetries) {
							std::wcout << L"[wait] " << c.name << L": installer process detected, waiting "
								<< L"(attempt " << (attempt + 1) << L"/" << maxRetries
								<< L", " << effectiveWaitSeconds << L"s interval";
							if (effectiveWaitSeconds != c.installerWaitSeconds) {
								std::wcout << L" (configured " << c.installerWaitSeconds << L"s)";
							}
							std::wcout << L")...\n";
							Sleep(sleepMs);
						}
					}
					if (!installerFinished) {
						std::wcout << L"[skip] " << c.name << L": installer still running after waiting\n";
						continue;
					}
					if (verbose) {
						std::wcout << L"[ ok ] " << c.name << L": installer finished, proceeding\n";
					}
				}
			}

			std::wcout << L"[run ] " << c.name << L"\n";

			std::int64_t startEpoch = ler::nowEpochSecondsUtc();
			ler::RunResult rr = ler::runProcess(c.exe, c.args, c.workingDirectory, c.timeoutSeconds);

			if (!rr.started) {
				std::wcerr << L"[fail] " << c.name << L": CreateProcessW failed (error=" << rr.exitCode << L")\n";
				overallExit = overallExit ? overallExit : 1;
				continue;
			}

			if (rr.timedOut) {
				std::wcerr << L"[fail] " << c.name << L": timed out; process terminated\n";
				overallExit = overallExit ? overallExit : 1;
			}
			else if (rr.exitCode != 0) {
				std::wcerr << L"[fail] " << c.name << L": exitCode=" << rr.exitCode << L"\n";
				overallExit = overallExit ? overallExit : static_cast<int>(rr.exitCode);
			}
			else {
				if (verbose) std::wcout << L"[ ok ] " << c.name << L": exitCode=0\n";
			}

			// Persist execution record (seconds precision).
			c.hasLastRunUtc = true;
			c.lastRunUtc = ler::formatEpochSecondsAsIsoUtc(startEpoch);
			c.hasLastExitCode = true;
			c.lastExitCode = rr.exitCode;
			cfg.dirty = true;
		}

		if (cfg.dirty) {
			ler::applyCommandsToJson(cfg);
			ler::writeWStringToUtf8FileAtomic(configPath, ler::writeJson(cfg.root));
		}

		return overallExit;
	}
	catch (const std::exception& ex) {
		std::string m = ex.what();
		std::wcerr << L"Fatal: " << std::wstring(m.begin(), m.end()) << L"\n";
		return 2;
	}
}

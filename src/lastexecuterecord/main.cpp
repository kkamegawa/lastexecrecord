#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>

#include "CommandRunner.h"
#include "Config.h"
#include "FileUtil.h"
#include "Json.h"
#include "TimeUtil.h"

static void printUsage(const wchar_t* exeName) {
	std::wcout
		<< L"LastExecuteRecord - run commands from JSON config once per invocation\n\n"
		<< L"Copyright (c) 2026 Kazushi Kamegawa\n\n"
		<< L"Usage:\n"
		<< L"  " << exeName << L" [--config <path>] [--dry-run] [--verbose]\n\n"
		<< L"Options:\n"
		<< L"  --config <path>  Path to config JSON (default: <exe>.json)\n"
		<< L"  --dry-run        Do not execute; only show decisions\n"
		<< L"  --verbose        Print skip reasons and detailed output\n";
}

int wmain(int argc, wchar_t* argv[]) {
	bool dryRun = false;
	bool verbose = false;
	std::wstring configPath = ler::defaultConfigPath();

	if (argc <= 1) {
		printUsage(argv[0]);
		return 0;
	}

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

		std::wcerr << L"Unknown argument: " << a << L"\n";
		printUsage(argv[0]);
		return 2;
	}

	try {
		// Prevent concurrent runs against the same config file.
		ler::FileLock lock = ler::acquireLockFile(configPath + L".lock");

		ler::AppConfig cfg = ler::loadAndValidateConfig(configPath);

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
					if (verbose) {
						std::wcout << L"[skip] " << c.name << L": minIntervalSeconds not reached (" << delta
							<< L"/" << c.minIntervalSeconds << L" sec)\n";
					}
					continue;
				}
			}

			std::wcout << L"[run ] " << c.name << L"\n";

			if (dryRun) {
				std::wcout << L"       exe: " << c.exe << L"\n";
				if (verbose && !c.args.empty()) {
					std::wcout << L"       args:";
					for (const auto& a : c.args) std::wcout << L" " << a;
					std::wcout << L"\n";
				}
				continue;
			}

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

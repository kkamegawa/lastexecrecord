# LastExecuteRecord Project

This project, named "LastExecuteRecord", is a Windows console application that executes commands defined in a JSON configuration file.
On each invocation, it runs commands sequentially and records the last execution time (seconds precision) back into the config.

The primary goal is to provide a simple, fast, and dependency-light way of running scheduled/guarded tasks using a single config file.

## Features

- Runs registered commands sequentially once per invocation.
- Records last execution time (UTC, seconds precision) and last exit code.
- Skips execution when minimum interval has not elapsed.
- Optional local-only pinning (prevents a copied config from running on other PCs).

## Getting Started

To get started with the LastExecuteRecord project, please refer to the main README.md in the repository root for config format and usage.

## Contribution

Contributions to the LastExecuteRecord project are welcome. Please refer to the project's contribution guidelines for more information on how you can contribute.

## License

This project is licensed under the MIT License - see the LICENSE file in the repository root for details.

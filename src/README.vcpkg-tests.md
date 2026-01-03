# Running tests (Visual Studio + vcpkg)

## Prerequisites

- Visual Studio 2022/2026 with C++ workload
- vcpkg installed and available on PATH

## One-time setup

From `src/`:

1. Install dependencies in manifest mode:

```powershell
vcpkg install --triplet x64-windows
```

2. Enable MSBuild integration (so `#include <gtest/gtest.h>` and linking works automatically):

```powershell
vcpkg integrate install
```

## Build and run

- Open `src/lastexecrecord.sln`.
- Build `lastexecuterecord.tests`.
- Open **Test Explorer** and run tests.

If tests do not appear, ensure that **GoogleTest Adapter** is installed/enabled in Visual Studio.

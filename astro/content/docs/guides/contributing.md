---
title: Contributing
description: Development workflow, branch/commit conventions, and code style.
---

Thanks for contributing. Consult the documentation in the repo's `docs/` folder and the
CubeMars manufacturer PDF before writing protocol code.

## Workflow

Development happens inside the dev container with `can-utils`, using CMake as the build
system. See [Dev Container](../../setup/dev-container/) and [Getting Started](../../setup/getting-started/).

## Branch naming

| Prefix        | Meaning               |
| ------------- | --------------------- |
| `feat/<name>` | New feature           |
| `fix/<name>`  | Fix for a known issue |
| `docs/<name>` | Documentation change  |

## Commit naming

| Message     | Description                         |
| ----------- | ----------------------------------- |
| `feat:`     | New feature                         |
| `fix:`      | Bug fix                             |
| `chore:`    | Maintenance/config, no logic change |
| `docs:`     | Documentation change                |
| `style:`    | Formatting/style, no logic change   |
| `refactor:` | Refactoring/restructuring           |
| `perf:`     | Performance change                  |
| `ci:`       | CI change                           |
| `revert:`   | Reverting a change                  |

## Code style

- Use leading return types, not trailing (`void doSomething()`, not `auto doSomething() -> void`), except for templates that require it.
- camelCase for functions and variables.
- Capitalize every word in `enum class` names and class names.

```cpp
enum class MitHeaderEnum {
    FrameDoesSomething = 0,
    FrameDoesSomethingElse
};
```

Most formatting is handled by the formatter on push to `main`.

## Testing

Use GTest. PRs to `main` must pass all tests, and new functionality must include tests in
the appropriate folders.

The testing workflow goes as such, and should be run inside the dev container

```bash title="Terminal"
cmake -S . -B build -DSETUP_TEST_IFNAME=ON -DBUILD_TESTING=ON
cd build
make
#run the test executable
./run_tests
```

This will build all of the files, make sure to include all of you C++ files in the cmake file in both of these spots

```cmake
target_sources(ak_series_lib
  PRIVATE
  src/<path>/<to>/file.cpp
)
```

```cmake
add_executable(run_tests
	  # Add testing files here once populated
      tests/<test-folder>/<test-file.cpp>
)
```

:::note
Use the same structure in the testing folder as the source folder, keeps the tests for each individual class easy to find
:::

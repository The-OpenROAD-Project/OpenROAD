# OpenROAD Testing Guide

OpenROAD has two types of tests: **integration tests** (Tcl) and **unit tests** (C++).

> For **where** new tests should live (the C++-first test pyramid) and **how** to
> handle DB/STA dependencies, see `testing-strategy.md`. This guide covers the
> mechanics of writing and registering a test.

## CAUTION -- Register Tests in Bazel

**Every new test MUST be registered in Bazel. A test missing from `BUILD` does not run in CI.**

CMake is deprecated and will be removed on Nov. 1, 2026. CMake test registration is not required.
New source files and new tools still need their CMake entries until then, because the macOS CI job builds with CMake and runs no `ctest`.

| Build System       | Registration File                  | Block/Macro                 |
| ------------------ | ---------------------------------- | --------------------------- |
| **Bazel**          | `src/<module>/test/BUILD`          | `regression_test(...)`      |
| CMake (deprecated) | `src/<module>/test/CMakeLists.txt` | `or_integration_tests(...)` |

See "Adding a Tool, Source File or Test in Bazel" in `docs/contrib/DeveloperGuide.md` for the `BUILD` patterns.

## Integration Tests

Integration tests are Tcl scripts located at `src/<module>/test` or `src/<module>/test/tcl`.

### Running Integration Tests

Each name in a test list of `src/<module>/test/BUILD` becomes a Bazel target `<name>-tcl_test`.
A Python test becomes `<name>-py_test`.

```bash
bazelisk test --test_output=errors //src/rsz/test:buffer_ports1-tcl_test   # one test
bazelisk test --test_output=errors //src/rsz/test/...                      # all rsz tests
bazelisk query 'filter("repair.*hier", //src/rsz/test:all)'                # find tests by regex
```

Bazel has no regex filter for test targets.
Run `bazelisk query 'filter(...)'` to list the matching targets, then pass them to `bazelisk test`.
`--test_filter` selects gtest cases in a C++ test only.

Legacy (CMake only, deprecated): `./regression` runs `cd build && ctest -L <module>`.
It needs a CMake build directory and does not see Bazel targets.

### Creating a New Integration Test

Best practice reference: `src/rsz/test/repair_tie12_hier.tcl`

#### Issue Reproduction Tests

For issue reproductions, keep the test case minimal and focused on the failing tool behavior. When floorplanning or placement is not part of the bug, encode the small problem design directly in DEF and use `read_def` after loading the necessary LEF and Liberty files instead of running floorplanning or global placement. This keeps the test simpler and more stable across unrelated OpenROAD behavior changes. Use floorplanning or placement steps when those stages are part of the behavior under test.

#### Checklist

1. **Header comment**: First line must explain the test purpose
   ```tcl
   # [Brief description of the test]
   ```

2. **Set test name**: Used for output file generation
   ```tcl
   set test_name <integration_test_name>
   ```

3. **Output naming with `make_result_file`**: Use for temporary output files
   ```tcl
   set verilog_filename "${test_name}.v"
   set out_verilog [make_result_file $verilog_filename]
   write_verilog $out_verilog
   ```

4. **Verification with `diff_file`**: Compare against golden files
   ```tcl
   diff_file ${test_name}.vok $out_verilog
   ```

5. **Generate `<test_name>.ok` file**: Create golden test log by executing the script, redirecting stdout, and removing the openroad banner at the top.

6. **Registration**: add `<test_name>` to the module's compulsory list (`COMPULSORY_TESTS` or `TESTS`) in `src/<module>/test/BUILD` (Bazel only; CMake test registration is not required). Another list (`PASSFAIL_TESTS`, `PY_TESTS`) also runs by default when the module's `ALL_TESTS` includes it, or its `test/BUILD` registers it, without a `manual` tag, so check the module's `test/BUILD`. Names tagged `manual` do not run in CI by default. Copy `data` and `tags` from a neighbouring entry, so `<test_name>.ok` and the inputs are in the runfiles

### Test Framework Flags
- `-no_splash -no_init -exit`

### Golden File Notes
- `.ok` = log golden file (full stdout), `.vok` = verilog golden file
- **`diff_file` reports only the first difference** -- when regenerating golden files, regenerate the entire output file rather than relying on incremental diffs.

## Unit Tests

Unit tests are C++ files located at `src/<module>/test/cpp`.

### Test Registration
- Bazel: `src/<module>/test/BUILD` or `src/<module>/test/cpp/BUILD`
  - Run with `bazelisk test //src/<module>/test:<name>` or `//src/<module>/test/cpp:<name>`, where `<name>` is the `cc_test` name.
- CMake (deprecated, test registration not required): `src/<module>/test/CMakeLists.txt`

## Python Unit Tests

Python binding tests can be plain `unittest` modules run as Bazel `py_test`
targets on the hermetic interpreter, one test method per behavior. The
example is `python/openroad/openroad_test.py`:

```starlark
load("@rules_python//python:defs.bzl", "py_test")

py_test(
    name = "openroad_test",
    size = "small",
    srcs = ["openroad_test.py"],
    deps = ["//python/openroad:openroadpy"],
)
```

`//python/openroad:openroadpy` provides `import openroad` and the per-tool
modules (`import odb`, `import ant`, ...) from `python/modules`, the same
names the wheel installs and `openroad -python` provides. Each tool module
has only its own API; all of them load from one library,
`_openroad.so`, so they share one copy of the C++ state. Keep each `Tech`
alive while its `Design` or the module-level functions (`set_thread_count`,
...) are in use.

### The openroad wheel

`bazelisk build //python/openroad:openroad_wheel` builds an abi3 wheel (CPython
3.10 or later) that installs into any virtualenv with `pip install`.
`//python/openroad:wheel_test` checks that it imports in a fresh interpreter
without the Bazel runfiles.

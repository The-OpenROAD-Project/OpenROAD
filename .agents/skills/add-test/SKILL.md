---
name: add-test
description: >
  Add integration or unit tests to an OpenROAD module. Handles the
  complete workflow: writing the Tcl/C++ test, generating golden files,
  and registering the test in BOTH CMake and Bazel build systems (the
  most common mistake is forgetting Bazel registration). Use when asked
  to add tests, improve test coverage, create regression tests, or write
  unit tests for any OpenROAD module (ant, cts, dpl, drt, gpl, grt, mpl,
  odb, pad, pdn, psm, rcx, rsz, sta, stt, tap, upf, utl, etc.).
  Also triggers on: "add test", "write test", "test coverage",
  "missing tests", "create regression test", "add unit test".
argument-hint: "<module> [test-description]"
---

# Add Test to OpenROAD Module

You are adding tests for: **$ARGUMENTS**

## 1. Identify the module and scope

Parse `$ARGUMENTS` to determine:
- **Module name** (e.g., `rsz`, `drt`, `gpl`)
- **What to test** (specific feature, edge case, or general coverage)

Explore existing tests to understand conventions:
```bash
ls src/MODULE/test/*.tcl | head -20
```

Read a representative test for the module's style:
```bash
# Good reference for integration test patterns:
cat src/rsz/test/repair_tie12_hier.tcl
```

## 2. Write the integration test (Tcl)

Create `src/MODULE/test/TEST_NAME.tcl`:

```tcl
# Brief description of what this test verifies
set test_name TEST_NAME
source "helpers.tcl"

# Read design files
read_lef "TECH.lef"
read_lef "CELLS.lef"
read_def "DESIGN.def"

# ... test operations ...

# Use make_result_file for temporary output
set def_filename "${test_name}.def"
set out_def [make_result_file $def_filename]
write_def $out_def

# Compare against golden file
diff_file ${test_name}.defok $out_def
```

### Test design files

Prefer reusing existing test data in `src/MODULE/test/`. Only create
new LEF/DEF files when testing a specific geometry or edge case that
existing files don't cover. Keep test data minimal.

### Naming conventions

- Test files: `src/MODULE/test/TEST_NAME.tcl`
- Golden log: `src/MODULE/test/TEST_NAME.ok`
- Golden output: `src/MODULE/test/TEST_NAME.{defok,vok,spefok,...}`
- Test data: `src/MODULE/test/*.{lef,def,lib,sdc,v}`

## 3. Generate golden files

Run the test and capture output:
```bash
cd src/MODULE/test

# Run with openroad to generate output
openroad -no_splash -no_init -exit TEST_NAME.tcl > TEST_NAME.ok 2>&1

# Remove the openroad banner from the .ok file if present
# The .ok file should contain only the test's stdout
```

If the test uses `diff_file`, also generate the golden output file
(e.g., `TEST_NAME.defok`) by running once and copying the result file.

## 4. Register in BOTH build systems

This is the most common mistake. **Every test must be in both CMake AND
Bazel.** CMake-only tests silently pass locally but are missing from
Bazel CI.

### CMake -- `src/MODULE/test/CMakeLists.txt`

Find the `or_integration_tests()` call and add the test name:
```cmake
or_integration_tests(
  TESTS
    existing_test1
    existing_test2
    TEST_NAME          # <-- add here
)
```

### Bazel -- `src/MODULE/test/BUILD`

Find the existing `regression_test()` entries and add:
```python
regression_test(
    name = "TEST_NAME",
)
```

Match the pattern used by neighboring tests in the same BUILD file --
some modules use additional attributes like `data` or `tags`.

## 5. Write unit tests (C++, if applicable)

For testing internal logic (not command-level behavior), add C++ unit
tests in `src/MODULE/test/cpp/`:

```cpp
#include <gtest/gtest.h>
#include "MODULE/src/ClassName.h"

TEST(ModuleTest, TestDescription) {
  // ...
  EXPECT_EQ(expected, actual);
}
```

Register in:
- `src/MODULE/test/CMakeLists.txt` -- as a C++ test target
- `src/MODULE/test/cpp/BUILD` -- as a Bazel `cc_test`

## 6. Run and verify

```bash
cd src/MODULE/test
./regression TEST_NAME

# Verify test is discoverable in both systems
cd ../../../build
ctest -N | grep MODULE.*TEST_NAME
```

## 7. Format and commit

```bash
# Format any C++ files (NEVER format src/sta/* or *.i files)
clang-format -i <changed-cpp-files>

git add src/MODULE/test/TEST_NAME.tcl \
        src/MODULE/test/TEST_NAME.ok \
        src/MODULE/test/CMakeLists.txt \
        src/MODULE/test/BUILD
git commit -s -m "MODULE: add TEST_NAME test

Test for DESCRIPTION."
```

## Checklist

- [ ] Test runs successfully: `./regression TEST_NAME`
- [ ] Golden file generated and committed
- [ ] Registered in CMake `CMakeLists.txt`
- [ ] Registered in Bazel `BUILD`
- [ ] Test data is minimal (reuse existing files when possible)
- [ ] C++ files formatted with clang-format
- [ ] Commit is signed off (`-s`)

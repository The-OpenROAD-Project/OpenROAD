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

Find the `or_integration_tests()` call. The first positional argument
is the **module name**, followed by the `TESTS` keyword and the list:

```cmake
or_integration_tests(
  "MODULE"           # <-- module name (e.g. "rsz")
  TESTS
    existing_test1
    existing_test2
    TEST_NAME        # <-- add here
)
```

Some modules also have a `PASSFAIL_TESTS` section after `TESTS` -- add
to the appropriate list. Look at the module's existing `CMakeLists.txt`
before editing.

### Bazel -- `src/MODULE/test/BUILD`

The typical pattern is a `TESTS` list consumed by a list comprehension
that calls `regression_test()` for each name:

```python
TESTS = [
    "buffer_ports1",
    "buffer_ports10",
    "buffer_ports11",
    "TEST_NAME",     # <-- add here, in the existing sort order
    # (other tests omitted)
]

[
    regression_test(
        name = test_name,
    )
    for test_name in TESTS
]
```

Some modules split tests across multiple lists (`TESTS`,
`PASSFAIL_TESTS`, `BIG_TESTS`, etc.) and define a combined
`ALL_TESTS = TESTS + PASSFAIL_TESTS + ...` that the comprehension
iterates over. Read the existing `BUILD` to find which list to
extend, and insert the new test name in whatever sort order that
file already uses (most modules are alphabetical).

## 5. Write unit tests (C++, if applicable)

Prefer C++ unit tests over Tcl for testing internal logic, algorithms,
and data structure operations. Use Tcl integration tests for
command-level behavior and end-to-end flows.

### Test file

Create `src/MODULE/test/cpp/TestName.cpp`:

```cpp
#include "gtest/gtest.h"
#include "odb/db.h"

// Simple test (no fixture)
namespace module {
namespace {

TEST(ModuleTest, TestDescription) {
  // ARRANGE - ACT - ASSERT
  EXPECT_EQ(expected, actual);
}

}  // namespace
}  // namespace module
```

### Using test fixtures

OpenROAD provides a fixture hierarchy for tests that need database
or tool setup. Choose the simplest one that covers your needs:

| Fixture | Use when |
|---------|----------|
| `tst::Fixture` | Need bare `dbDatabase` + STA, load LEFs manually |
| `tst::Nangate45Fixture` | Need pre-loaded Nangate45 tech/lib |
| `tst::Sky130Fixture` | Need pre-loaded Sky130 tech/lib |
| `tst::IntegratedFixture` | Need full tool integration (STA, DPL, GRT, RSZ) |

```cpp
#include "gtest/gtest.h"
#include "tst/nangate45_fixture.h"

class TestFeature : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override { /* additional setup */ }
};

TEST_F(TestFeature, HandlesEdgeCase) {
  // block_, lib_ are available from the fixture
  EXPECT_TRUE(condition);
}
```

### CMake registration -- `src/MODULE/test/cpp/CMakeLists.txt`

```cmake
add_executable(TestName TestName.cpp)
target_link_libraries(TestName
    MODULE_lib
    GTest::gtest
    GTest::gtest_main
    tst
    odb
)
gtest_discover_tests(TestName
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)
add_dependencies(build_and_test TestName)
```

### Bazel registration -- `src/MODULE/test/BUILD`

Add the `cc_test` target alongside the existing `regression_test`
entries. Most modules keep C++ tests in the same BUILD file:

```python
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "TestName",
    srcs = ["cpp/TestName.cpp"],
    deps = [
        "//src/MODULE",
        "//src/tst",
        "//src/tst:nangate45_fixture",  # if using Nangate45Fixture
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
```

## 6. Run and verify

`./regression` is a thin ctest wrapper that filters by module label, so
the standard ctest flags apply. Use `-R` to match a single test by name:

```bash
cd src/MODULE/test
./regression -R TEST_NAME

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

- [ ] Test runs successfully: `./regression -R TEST_NAME`
- [ ] Golden file generated and committed
- [ ] Registered in CMake `CMakeLists.txt`
- [ ] Registered in Bazel `BUILD`
- [ ] Test data is minimal (reuse existing files when possible)
- [ ] C++ files formatted with clang-format
- [ ] Commit is signed off (`-s`)

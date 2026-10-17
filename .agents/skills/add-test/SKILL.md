---
name: add-test
description: >
  Add integration or unit tests to an OpenROAD module. Handles the
  complete workflow: writing the Tcl/C++ test, generating golden files,
  and registering the test in Bazel (CMake is deprecated and CMake
  test registration is not required). Use when asked
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

# Run with the Bazel-built binary (bazelisk build //:openroad) to generate output
../../../bazel-bin/openroad -no_splash -no_init -exit TEST_NAME.tcl > TEST_NAME.ok 2>&1

# Remove the openroad banner from the .ok file if present
# The .ok file should contain only the test's stdout
```

If the test uses `diff_file`, also generate the golden output file
(e.g., `TEST_NAME.defok`) by running once and copying the result file.

## 4. Register in Bazel

Register every test in Bazel. A test missing from `BUILD` does not run in
CI. CMake is deprecated and will be removed on Nov. 1, 2026, so CMake
test registration is not required. This applies to tests only. A new
source file or tool still needs its CMake entry until CMake is removed,
because the macOS CI job builds with CMake (it runs no `ctest`).

### Bazel -- `src/MODULE/test/BUILD`

Most modules list the tests in `COMPULSORY_TESTS`, and some use `TESTS`.
A list comprehension calls `regression_test()` for each name. Add the
test to the module's compulsory list (`COMPULSORY_TESTS` or `TESTS`).
Another list (`PASSFAIL_TESTS`, `PY_TESTS`) also runs by default when the
module's `ALL_TESTS` includes it, or its `test/BUILD` registers it, without a
`manual` tag. Check the module's `test/BUILD`. Names tagged `manual` do not run in CI by default.

```python
COMPULSORY_TESTS = [
    "buffer_ports1",
    "TEST_NAME",     # <-- add here, in the existing sort order
    # (other tests omitted)
]

ALL_TESTS = COMPULSORY_TESTS  # some modules add other lists here

[
    regression_test(
        name = test_name,
        data = [":" + test_name + "_resources"],  # copy from the module
        tags = [] if test_name in COMPULSORY_TESTS else ["manual"],
    )
    for test_name in ALL_TESTS
]
```

Copy the `data` and `tags` arguments from the existing call in the same
`BUILD` file. The macro adds the test file, `//test:regression_resources`,
the OpenROAD binary and `data` to the runfiles. It does not add
`TEST_NAME.ok` or the input files. The module's `data` filegroup must
carry them. If it lists input files by name, add yours there. See
`src/exa/test/BUILD` (one `test_resources` filegroup with a broad glob)
and `src/ant/test/BUILD` (one `<name>_resources` filegroup per test).

`regression_test()` creates the target `TEST_NAME-tcl_test` for a `.tcl`
test and `TEST_NAME-py_test` for a `.py` test.

Some modules split tests across multiple lists (`COMPULSORY_TESTS`,
`PASSFAIL_TESTS`, `BIG_TESTS`, etc.) and define a combined
`ALL_TESTS = COMPULSORY_TESTS + PASSFAIL_TESTS + ...` that the
comprehension iterates over. Read the existing `BUILD` to find which list
to extend, and insert the new test name in whatever sort order that
file already uses (most modules are alphabetical).

### CMake (deprecated, not required)

Skip `src/MODULE/test/CMakeLists.txt`. The macOS CI job runs no `ctest`.

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

### Bazel registration -- `src/MODULE/test/BUILD`

Add the `cc_test` target alongside the existing `regression_test`
entries. Most modules keep C++ tests in the same BUILD file; some use
`src/MODULE/test/cpp/BUILD` (for example `src/odb/test/cpp/BUILD`).
Keep the paths the module already uses:

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

CMake test registration is not required. The deprecated CMake build uses
`add_executable`, `gtest_discover_tests`, and `add_dependencies` in
`src/MODULE/test/cpp/CMakeLists.txt`.

## 6. Run and verify

Run a Tcl or Python test by its Bazel target name. Add
`--test_output=errors` to see the log of a failing test:

```bash
bazelisk test --test_output=errors //src/MODULE/test:TEST_NAME-tcl_test

# C++ unit test: the target name is the cc_test name
bazelisk test //src/MODULE/test:TestName

# Run all tests of the module
bazelisk test //src/MODULE/test/...

# Verify Bazel sees the test
bazelisk query //src/MODULE/test:all | grep TEST_NAME
```

Legacy (CMake only, deprecated): `./regression -R TEST_NAME` in
`src/MODULE/test` runs `ctest` in `build/`. It does not see Bazel
targets.

## 7. Format and commit

```bash
# Format any C++ files (NEVER format src/sta/* or *.i files)
clang-format -i <changed-cpp-files>

git add src/MODULE/test/TEST_NAME.tcl \
        src/MODULE/test/TEST_NAME.ok \
        src/MODULE/test/BUILD
git commit -s -m "MODULE: add TEST_NAME test

Test for DESCRIPTION."
```

## Checklist

- [ ] Test runs successfully: `bazelisk test //src/MODULE/test:TEST_NAME-tcl_test`
- [ ] Golden file generated and committed
- [ ] Registered in Bazel `BUILD`
- [ ] CMake test registration is skipped (not required); new source files or tools still need their CMake entry
- [ ] Test data is minimal (reuse existing files when possible)
- [ ] C++ files formatted with clang-format
- [ ] Commit is signed off (`-s`)

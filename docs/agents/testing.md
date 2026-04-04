# OpenROAD Testing Guide

OpenROAD has two types of tests: **integration tests** (Tcl) and **unit tests** (C++).

## CAUTION -- Dual Registration Required

**Every new test MUST be registered in BOTH build systems. Missing either one is a build break.**

| Build System | Registration File | Block/Macro |
|-------------|-------------------|-------------|
| **CMake** | `src/<module>/test/CMakeLists.txt` | `or_integration_tests(...)` |
| **Bazel** | `src/<module>/test/BUILD` | `regression_test(...)` |

Forgetting the Bazel `BUILD` file is the most common mistake -- CMake-only registration
silently passes local `make test` but the **test will be missing from Bazel CI**.

## Integration Tests

Integration tests are Tcl scripts located at `src/<module>/test` or `src/<module>/test/tcl`.

### Running Integration Tests

`./regression` is a ctest wrapper that filters by module label.

```bash
cd src/rsz/test
./regression -R buffer_ports1   # run one test (regex match)
./regression -R "repair.*hier"  # run tests matching pattern
./regression -j4 -V             # all rsz tests, parallel, verbose

# Or equivalently from the build directory:
cd build
ctest -R "rsz\.buffer_ports1\.tcl"
```

### Creating a New Integration Test

Best practice reference: `src/rsz/test/repair_tie12_hier.tcl`

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

6. **Registration (BOTH files for CMake and Bazel -- do NOT skip Bazel)**

### Test Framework Flags
- `-no_splash -no_init -exit`

### Golden File Notes
- `.ok` = log golden file (full stdout), `.vok` = verilog golden file
- **`diff_file` reports only the first difference** -- when regenerating golden files, regenerate the entire output file rather than relying on incremental diffs.

## Unit Tests

Unit tests are C++ files located at `src/<module>/test/cpp`.

### Test Registration
- CMake: `src/<module>/test/CMakeLists.txt`
- Bazel: `src/<module>/test/BUILD` or `src/<module>/test/cpp/BUILD`

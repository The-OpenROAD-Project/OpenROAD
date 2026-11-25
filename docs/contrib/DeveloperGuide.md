# Developer Guide

## Tool Philosophy

OpenROAD is a tool to build a chip from synthesizable RTL (Verilog) to
completed physical layout (manufacturable, tapeout-clean GDSII).

The unifying principle behind the design of OpenROAD is for all of the
tools to reside in one tool, with one process, and one database. All
tools in the flow should use Tcl commands exclusively to control them
instead of external "configuration files". File-based communication
between tools and forking processes is strongly discouraged. This
architecture streamlines the construction of a flexible tool flow and
minimizes the overhead of invoking each tool in the flow.

## Tool File Organization

:::{Note}
Bazel is the only build system for OpenROAD. Register new source files, tools
and tests in Bazel `BUILD` files.
:::

Every tool follows the following file structure, grouping sources, tests
and headers together.

- `openroad/`
   This folder contains the source files for individual tools.

| `openroad`      | Purpose |
|-----------------|--------------|
| `tool/BUILD`    | Bazel targets of the tool: `cc_library`, `tcl_encode`, `tcl_wrap_cc` |
| `tool/test/BUILD` | Bazel tests of the tool: `regression_test`, `cc_test` |
| `tool/src`       | sources and private headers |
| `tool/include/tool` | exported headers |
| `tool/test`       | tool tests |

- OpenROAD repository:
  This folder contains the top-level files for overall compilation. OpenROAD uses [swig](https://swig.org/) that acts as a wrapper for C/C++ programs to be callable in higher-level languages, such as Python and Tcl.

| `OpenROAD`      | Purpose |
|-----------------|--------------|
| `BUILD.bazel`, `MODULE.bazel`, `.bazelrc` | top-level Bazel targets, dependencies and shared flags |
| `openroad/Main.cc`   | main file  |
| `openroad/OpenRoad.cc` | OpenROAD class functions |
| `openroad/OpenRoad.i`  | top-level swig, includes, tool swig files |
| `openroad/OpenRoad.tcl` | basic read/write lef/def/db commands |
| `include/ord/OpenRoad.hh` | OpenROAD top-level class, has instances of tools |

Some tools such as OpenSTA are submodules, which are simply
subdirectories in `openroad/` that are pointers to the git submodule. They are
intentionally not segregated into a separate module.

The use of submodules for new code integrated into OpenROAD is strongly
discouraged. Submodules make changes to the underlying infrastructure
(e.g., OpenSTA) difficult to propagate across the dependent submodule
repositories.

Where external/third-party code that a tool depends on should be placed
depends on the nature of the dependency.

-   Libraries - code packaged as a linkable library. Examples are `tcl`,
    `boost`, `zlib`, `eigen`, `lemon`, `spdlog`.

Add each library as a `bazel_dep` in `MODULE.bazel` and list it in the
`deps` of the tool target that uses it. Document these dependencies in the
top-level `README.md` file.

Each tool `BUILD` file defines a `cc_library` that is linked by the OpenROAD
application. The tools should not define a `main()` function.
Global compiler flags live in `.bazelrc`, so a tool `BUILD` file should not
repeat them. `OPENROAD_COPTS` in the top-level `BUILD.bazel` applies to the
top-level targets only. A tool may add per-target `copts` for a local need,
for example `-Wno-missing-braces` for SWIG-generated code in `openroad/pdn/BUILD`.

None of the tools have commands to read or write LEF, DEF, Verilog or
database files. For consistency, these functions are all provided by the OpenROAD
framework.

Tools should package all of their state in a single class. An instance of each
tool class resides in the top-level OpenROAD object. This allows
multiple tools to exist at the same time. If any tool keeps state in
global variables (even static), then only one tool can exist at a time. Many
of the tools being integrated were not built with this goal in mind and
will only work on one design at a time.

Each tool should use a unique namespace for all of its code. The same
namespace should be used for Tcl functions, including those defined by a
swig interface file. Internal Tcl commands stay inside the namespace,
and user visible Tcl commands should be defined in the global namespace.
User commands should be simple Tcl commands such as `global_placement`
that do not create tool instances that must be based to the commands.
Defining Tcl commands for a tool class is fine for internal commands, but not
for user visible commands. Commands have an implicit argument of the
current OpenROAD class object. Functions to get individual tools from
the OpenROAD object can be defined.

## Initialization (C++ tools only)

The OpenROAD class has pointers to each tool, with functions to get each
tool. Each tool has (at a minimum) a function to make an instance of the
tool class, an initialization function that is called after all of
the tools have been made, and a function to delete the tool. This small
header does **not** include the class definition for the tool so that
the OpenROAD framework does not have to know anything about the tool
internals or include a gigantic header file.

`MakeTool.hh` defines the following:

``` cpp
Tool *makeTool();
void initTool(OpenRoad *openroad);
void deleteTool(Tool *tool);
```

The `OpenRoad::init()` function calls all of the `makeTool` functions and
then all of the `initTool()` functions. The `init` functions are called from
the bottom of the tool dependencies. Each `init` function grabs the state
it needs out of the `OpenRoad` instance.

## Commands

Tools should provide Tcl commands to control them. Tcl object based tool
interfaces are not user-friendly. Define Tcl procedures that take
keyword arguments that reference the `OpenRoad` object to get tool state.
OpenSTA has Tcl utilities to parse keyword arguments
(`sta::parse_keyword_args`). See `OpenSTA/tcl/*.tcl` for
examples. Use swig to define internal functions to C++ functionality.

Tcl files can be included by encoding them into a string that
is evaluated at run time. Bazel does this with the `tcl_encode` rule
(for example, the `tcl` target in `openroad/rsz/BUILD`), and the tool evaluates
the string with `utl::evalTclInit` (see
[`initResizer()`](../main/openroad/rsz/src/MakeResizer.cc)).

:::{Note}
Please refer to the top-level Tcl formatting [guide](TclFormat.md).
Our top-level Tcl files, in particular, have to be formatted in this specific
manner because of the automatic parsing used to convert the READMEs into
manpages.
:::

## Errors

Tools should report errors to the user using the logging infrastructure
described in the [Logger guide](Logger.md). The `utl::error` function
throws an exception that is caught at the Tcl command level. The variables
`exit_on_error` and `file_continue_on_error` control how the error is
handled. If `exit_on_error` is `1` then OpenROAD reports the error and
exits. If the error is encountered while reading a file with the `source`
or `read_sdc` commands and `file_continue_on_error` is `0` then no other
commands are read from the file.

## Support for Design Space Exploration (DSE) & Automated Tools

OpenROAD supports automated Design Space Exploration (DSE) tools and autotuners across all stages of physical design. Automated tools run across a wide spectrum:
* **Early RTL & Floorplan Exploration**: Fast feedback loops using custom project Tcl script snippets developed for a specific phase in a project.
* **Late-Stage Pre-Tapeout Sweeps**: Full, unmodified flow runs through final parasitic extraction (`rcx`) to eke out a few percent improvement in timing and area via seed and parameter sweeps.

For a comprehensive guide on DSE orchestration, managing expectations with optimization functions, early-stopping strategies, and framework inversion, see the [Design Space Exploration Guide](./DesignSpaceExploration.md).

To support automated workflows, optimization passes must follow these guidelines for non-convergent ("doomed") runs:

### 1. Zero-Config Futility Guards
Optimization passes (e.g., gate sizing, buffer insertion, congestion repair) must detect when an iteration loop diverges or hits diminishing returns.
* Futility guards **must be automatic and zero-config**. They must not require CLI options or user parameters.
* Automated tools encounter futile states when exploring unclosable RTL or searching outside the valid solution domain. Users cannot configure options for unknown failure modes.

### 2. Pass Termination Without Flow Interruption
When an optimization pass hits a futility limit:
* The pass must **stop its internal iteration loop** and issue a clear warning (`utl::warn`).
* The overall flow **must run to completion**.

### 3. Downstream Metric Evaluation
Small upstream variations (e.g., placement seed changes) cause large downstream effects ("placement lottery"). Accurate clock period and parasitic-aware timing signals often emerge late in the flow:
* Global routing (`grt`) provides realistic clock period estimates.
* Detailed routing (`drt`) and parasitic extraction (`rcx`) resolve local congestion and extract wire parasitics.

Completing the flow allows autotuners to collect final metrics and extract actionable signals, even when an intermediate pass halts early.

## Test

Each "tool" has a `/test` directory with its tests. Tests are registered
in `openroad/<tool>/test/BUILD` and run with Bazel (see
[Adding a Tool, Source File or Test in Bazel](#adding-a-tool-source-file-or-test-in-bazel)).

``` shell
# all tests of one tool
bazelisk test //openroad/<tool>/test/...

# one Tcl test; the target is <test name>-tcl_test
bazelisk test --test_output=errors //openroad/<tool>/test:<test name>-tcl_test
```

No database files should be in tests. Read LEF/DEF/Verilog to make a
database.

The Nangate45 open-source library data used by many tests is in
`test/Nangate45`. Use the following command to add a link in the tool command:

``` shell
cd openroad/<tool>/test
ln -s ../../../test/Nangate45
```

After the link is installed, the test script can read the Liberty file
with the command shown below.

``` tcl
read_liberty Nangate45/Nangate45_typ.lib
```

### Create a minimal Tcl test

Before writing a golden-file Tcl test, check `docs/agents/testing-strategy.md`.
New tests default to C++ unit tests (`cc_test`), with thin `PASSFAIL_TESTS`
for Tcl binding smoke tests and `unittest` `py_test` targets for Python
binding smoke tests.
Use a golden-file Tcl test only when the behavior spans tools or cannot be
expressed with a C++ fixture such as `tst::DbFixture::readDef`.

Keep a new Tcl test small.
For a bug report, put the small design in a DEF file and read it with
`read_def` after the LEF and Liberty files, so the test does not depend on
floorplanning or placement.
Use those steps only when they are part of the behavior under test.

1. Reduce the design until it still shows the behavior.
   Write it as `openroad/<tool>/test/<name>.def`.
2. Write `openroad/<tool>/test/<name>.tcl`, starting with a one-line comment
   explaining what it tests.
   For example, `openroad/gpl/test/incremental01.tcl` reads a LEF and a DEF, runs the
   command, writes a DEF, and compares it with a golden file:

   ```tcl
   source helpers.tcl
   set test_name incremental01
   read_lef ./nangate45.lef
   read_def ./$test_name.def

   global_placement -init_density_penalty 0.1 -incremental
   set def_file [make_result_file $test_name.def]
   write_def $def_file
   diff_files $def_file $test_name.defok
   ```

   `make_result_file` puts the file in the results directory and adds `-tcl`
   to its name, so `incremental01.def` becomes `incremental01-tcl.def`.
3. Where the `BUILD` file lists golden files by name, create empty `<name>.ok`
   and, if the test calls `diff_files` on a DEF, `<name>.defok` files so Bazel
   can find the declared inputs.
4. Register `<name>` in `openroad/<tool>/test/BUILD` (see "Add a test" under
   "Adding a Tool, Source File or Test in Bazel" below).
5. Run the registered test and review its output, then update the golden files
   as described in the next section.

### Update golden files

The log golden file `<name>.ok` holds the full output of the test.
The DEF golden file `<name>.defok` holds the DEF that the test writes.
When a change gives a correct new result, review the difference, then replace
the golden file.

Most tool test directories have a `save_ok` link, and many have a `save_defok`
link, pointing to `test/shared/save_ok.sh` and `test/shared/save_defok.sh`.
(`openroad/grt/test` also provides `save_guideok` for `.guideok` files.)
Run them in `openroad/<tool>/test` and pass test names without extension.
In directories without a local link (for example, `openroad/syn/test` has
neither link, and `openroad/cts/test` has `save_ok` but no `save_defok`), run
`../../../test/shared/save_ok.sh <name>` or
`../../../test/shared/save_defok.sh <name>` directly from `openroad/<tool>/test`.
Each script runs `bazel test` for `//<package>:<name>-tcl_test` (falling back
to `-py_test` if no Tcl output is found) and copies the result file from
`bazel-testlogs` with writable `0644` permissions.
The `bazel` command must be on `PATH`.
The test is expected to fail while the golden file is old.

When a test writes a DEF whose filename matches `<name>`, update its DEF golden
file first, then its log golden file:

``` shell
cd openroad/<tool>/test
./save_defok <name>
./save_ok <name>
```

`save_ok` runs the test again, so the saved log records the comparison with the
new DEF golden file.
For a test that has no other golden file, run only `save_ok`.

The save helpers assume the output filename matches the test stem
(`<name>-tcl.<ext>` to `<name>.<dest_ext>`).
When a test writes a differently named output (such as `simple_test_out.defok`)
or another golden type without a helper (such as `.vok`), copy each generated
file from
`bazel-testlogs/openroad/<tool>/test/<name>-tcl_test/test.outputs/results/`
to the exact golden path in the test before refreshing `<name>.ok`.
Bazel writes test outputs with read-only executable permissions (`0555`), so
use `install -m 644` (or `rm -f`, `cp`, and `chmod 644`) instead of plain `cp`:

``` shell
install -m 644 \
  ../../../bazel-testlogs/openroad/<tool>/test/<name>-tcl_test/test.outputs/results/<result-file> \
  <golden-file>
```

Some Bazel versions pack these files in `test.outputs/outputs.zip` instead;
extract them with
`unzip -p <outputs.zip> results/<result-file> > <golden-file>` and set mode
`0644`.

Check the result with `git diff` before you commit.
`diff_files` reports the first difference in the files it compares.
The save scripts replace the complete golden files with the new output.
Rerun every registered target that uses the updated golden file to confirm
that it passes (both `-tcl_test` and `-py_test` if both exist):

``` shell
bazelisk test --test_output=errors //openroad/<tool>/test:<name>-tcl_test
bazelisk test --test_output=errors //openroad/<tool>/test:<name>-py_test
```

### Update flow metrics and limits

The flow tests in the top-level `test` directory, such as
`aes_sky130hd` and `gcd_nangate45`, run a whole flow inside OpenROAD.
They are long-running and have no golden log.
Each one compares its result metrics against the thresholds in
`test/<name>.metrics_limits`, and fails when a metric violates its threshold.
The file `test/<name>.metrics` holds the saved result metrics.
These tests are tagged `manual`, so name the target to run one (or use
`//test:flow_tests` to run all of them):

``` shell
bazelisk test --test_output=errors //test:aes_sky130hd-tcl_test
```

When a change moves metrics on purpose, review the new results, refresh the
saved metrics, and update only the limits that the change justifies:

1. Create `test/results/` and copy the result metrics file with `0644`
   permissions (Bazel writes `bazel-testlogs` files with mode `0555`, which
   `save_flow_metrics` preserves):

   ``` shell
   mkdir -p test/results
   install -m 644 \
     bazel-testlogs/test/aes_sky130hd-tcl_test/test.outputs/results/aes_sky130hd-tcl.metrics \
     test/results/aes_sky130hd-tcl.metrics
   ```

   If your Bazel version packs outputs into `test.outputs/outputs.zip`, extract
   the file with
   `unzip -p bazel-testlogs/test/aes_sky130hd-tcl_test/test.outputs/outputs.zip results/aes_sky130hd-tcl.metrics > test/results/aes_sky130hd-tcl.metrics`
   and set mode `0644`.
2. Update `<name>.metrics` from the `test` directory:

   ``` shell
   cd test
   ./save_flow_metrics aes_sky130hd
   ```

   If a threshold in `<name>.metrics_limits` also needs to change, either edit
   that threshold directly or run `./save_flow_metrics_limits aes_sky130hd`
   (which needs the Tcl `json` package from Tcllib).
   `save_flow_metrics_limits` regenerates every limit in the file from the
   current run plus a margin, including DRC and antenna limits, so check
   `git diff` and keep only the limit changes your change justifies.
3. Rerun the named flow test to confirm it passes, review `git diff`, and
   commit the updated files.

Running `./save_flow_metrics` with no test name requires every flow test's
`.metrics` file to be present in `test/results/`.
The `test/results/` directory is ignored by git.

## Building

Instructions for building are available [here](../user/Build.md).
The Bazel binary is `bazel-bin/openroad/openroad`.

## Debug with gdb

Build with debug information from the repository root, matching the failing
target's build configuration (for example, GPU build flags for GPU tests):

``` shell
bazelisk build -c dbg //:openroad
```

Start `gdb` from the test directory so the test can find its helper and input files:

``` shell
cd openroad/<tool>/test
```

Use the regression runner's single-thread setting and exit behavior, and set
any environment variables the failing target sets (for example `ENABLE_GPU=1`
for GPU tests, or `ENABLE_GPU=0` for CPU regressions on a GPU build):

``` shell
gdb --args ../../../bazel-bin/openroad/openroad -no_init -no_splash -exit -threads 1 <name>.tcl
```

On macOS, run `lldb --` with the same binary and arguments.
To keep the OpenROAD Tcl prompt after the script, omit `-exit`.
If the failure depends on a different thread count, set `-threads` to that count.

`bazel-bin` points to the most recent build.
If you switch between build modes, use the path in `bazel-out/` instead.
To run a `-c dbg` OpenROAD binary with ORFS, see the "Testing an OpenROAD
build with ORFS from within the OpenROAD folder" section in
[the Bazel guide](../user/Bazel.md).

## Adding a Tool, Source File or Test in Bazel

Tools, source files and tests are registered in Bazel `BUILD` files. A test
that is missing from a `BUILD` file does not run in CI. Copy a nearby target
and keep visibility minimal.

### Add a test

Each tool lists its Tcl and Python tests by name in `openroad/<tool>/test/BUILD`.
Most modules keep them in a `COMPULSORY_TESTS` list (some use `TESTS`), and
some add `PASSFAIL_TESTS` or `BIG_TESTS`. Add the file
`openroad/<tool>/test/<name>.tcl` (or `.py`) and its golden `<name>.ok`, then add
`<name>` to the module's compulsory list (`COMPULSORY_TESTS` or `TESTS`).
Another list, such as `PASSFAIL_TESTS` or `PY_TESTS`, also runs by default
when the module's `ALL_TESTS` includes it, or its `test/BUILD` registers it,
without a `manual` tag. Check the module's `test/BUILD`. Names tagged
`manual` do not run in CI by default.

The `regression_test` macro from `//test:regression.bzl` creates the target
`<name>-tcl_test` for `<name>.tcl` and `<name>-py_test` for `<name>.py`, and
tags it `tcl` or `py`. It adds the test file, `//test:regression_resources`,
the OpenROAD binary and the `data` you pass to the test runfiles. It does not
add `<name>.ok` or the input files of the test. The `data` argument must carry
them, so each module defines a filegroup and passes it as `data`.

For example, `openroad/exa/test/BUILD` registers the `basic` test with:

```starlark
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "basic",
]

filegroup(
    name = "test_resources",
    # All files in the package except the test scripts themselves, so the
    # .ok file and the inputs are in the runfiles.
    srcs = glob(
        ["**/*"],
        exclude = [
            test + "." + ext
            for test in TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[regression_test(
    name = test_name,
    data = [":test_resources"],
) for test_name in TESTS]
```

Other modules build the filegroup per test, for example `openroad/ant/test/BUILD`
has a `<name>_resources` filegroup with `glob([test_name + ".*"])` plus the
input files. When you add a test to a module, copy `data` and `tags` from a
neighbouring entry and add your input files to its resources.

This gives `//openroad/exa/test:basic-tcl_test` and
`//openroad/exa/test:basic-py_test`. A C++ unit test is a `cc_test` in the same
`BUILD` file (for example `//openroad/rsz/test:TestBufRem1`) or in
`openroad/<tool>/test/cpp/BUILD` (for example `openroad/odb/test/cpp/BUILD`).
List the `srcs` and `deps` explicitly, including `@googletest//:gtest` and
`@googletest//:gtest_main`.

Check that Bazel sees the test, then run it:

``` shell
bazelisk query 'kind(test, //openroad/exa/test/...)'
bazelisk test --test_output=errors //openroad/exa/test:basic-tcl_test
```

### Add a source file

Add the file to the `srcs` of the tool `cc_library` in `openroad/<tool>/BUILD`.
Add a header that other tools include to `hdrs`. For example, `openroad/pdn/BUILD`
lists `src/via_repair.cpp` and `src/via_repair.h` in `srcs` of the `pdn`
target, and `include/pdn/PdnGen.hh` in `hdrs`. Add a new Tcl file to the `srcs`
of the tool `tcl_encode` target (`src/pdn.tcl` in the `tcl` target of
`openroad/pdn/BUILD`). Then build:

``` shell
bazelisk build //:openroad
```

### Add a tool

Use `openroad/exa` as the model; the walkthrough
[below](#example-of-adding-a-tool-to-openroad) describes it. In Bazel, copy the
structure of `openroad/exa/BUILD` or `openroad/pdn/BUILD`: a `cc_library` for the sources,
a `tcl_encode` target for the Tcl commands, a `tcl_wrap_cc` target for the swig
interface, a `python_wrap_cc` and `python_wrap_library` pair for the Python
binding (`swig-py` and `py_wrap`), and a `messages_txt` target for the message
list. Then wire the tool into the build and the binary:

1. Add the tool targets to `OPENROAD_LIBRARY_DEPS` in the top-level
   `BUILD.bazel` (`//openroad/exa` and `//openroad/exa:ui` are an example).
2. Add `//openroad/<tool>:py_wrap` to the `deps` of `_openroad.so`, and
   `//openroad/<tool>:swig-py` to the `srcs` of `openroad_py`, in `BUILD.bazel`.
3. Add `//openroad/<tool>:messages_txt` to `messages` and `//openroad/<tool>:README.md`
   to `readmes` in `docs/BUILD.bazel`, and export the README from the tool
   `BUILD` with `exports_files(["README.md"])`.
4. Add `X(<tool>)` to `FOREACH_TOOL_WITHOUT_OPENROAD` in `openroad/Main.cc` so the
   Python module `_<tool>_py` is initialized.
5. In `include/ord/OpenRoad.hh`, forward declare the tool class, add a getter
   and a member pointer. In `openroad/OpenRoad.cc`, include the `Make<Tool>.h`
   header and the tool class header (for exa: `exa/MakeExample.h` and
   `exa/example.h`), create the object in `OpenRoad::init`, delete it in the
   destructor, and call the tool `init` function with the Tcl interpreter (for
   exa: `exa::initExample`). Add a getter to `openroad/OpenRoad.i` only when other
   swig interfaces need it (`getPdnGen` is an example). The exa swig file calls
   `ord::OpenRoad::openRoad()->getExample()` itself.
6. For a tool with Python bindings, add `python/modules/<tool>.py` (copy
   `python/modules/exa.py`), add the tool name to `_TOOLS` in
   `python/modules/_openroad_loader.py`, and import it in
   `python/openroad/__init__.py`.
7. Add the tool README to `docs/toc.yml` (see the `main/openroad/exa/README` entry).

## Example of Adding a Tool to OpenROAD

The directory [openroad/exa](../main/openroad/exa/README.md)
illustrates a tool named "Example" that uses the file structure described above.
It defines a command to run the tool as illustrated below:

```tcl
> example_instance -name test
[INFO EXA-0001] Making an example instance named test

> help example_instance
example_instance [-name name] 
```

The example include TCL & Python APIs with unit tests.  Also demonstrated is how to create debug graphics for algorithm visualization.  Running `openroad -gui basic.tcl` in `openroad/exa/test` will demonstrate the capability.

## Documentation

Tool commands should be documented in the top-level OpenROAD `README.md`
file. Detailed documentation should be the `tool/README.md` file.

:::{Note}
Please refer to the README formatting [guide](ReadmeFormat.md).
Our top-level READMEs, in particular, have to be formatted in this specific
manner because of the automatic parsing used to convert the READMEs into
manpages.
:::

## Tool Flow Namespace

Tool namespaces are usually three-lettered lowercase letters.

- Verilog to DB (dbSTA)
- OpenDB: Open Database ([odb](../main/openroad/odb/README.md))
- TritonPart: constraints-driven paritioner ([par](../main/openroad/par/README.md))
- Floorplan Initialization ([ifp](../main/openroad/ifp/README.md))
- ICeWall chip-level connections ([pad](../main/openroad/pad/README.md))
- I/O Placement ([ppl](../main/openroad/ppl/README.md))
- PDN Generation ([pdn](../main/openroad/pdn/README.md))
- Tapcell and Welltie Insertion ([tap](../main/openroad/tap/README.md))
- Macro Placer ([mpl](../main/openroad/mpl/README.md))
- RePlAce Global Placer ([gpl](../main/openroad/gpl/README.md))
- Gate resizing and buffering ([rsz](../main/openroad/rsz/README.md))
- Parasitics estimation ([est](../main/openroad/est/README.md))
- Detailed placement ([dpl](../main/openroad/dpl/README.md))
- Clock tree synthesis ([cts](../main/openroad/cts/README.md))
- FastRoute Global routing ([grt](../main/openroad/grt/README.md))
- Antenna check and diode insertion ([ant](../main/openroad/ant/README.md))
- TritonRoute Detailed routing ([drt](../main/openroad/drt/README.md))
- Metal fill insertion ([fin](../main/openroad/fin/README.md))
- Design for Test ([dft](../main/openroad/dft/README.md))
- OpenRCX Parasitic Extraction ([rcx](../main/openroad/rcx/README.md))
- OpenSTA timing/power analyzer ([sta](https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/README.md))
- Graphical User Interface ([gui](../main/openroad/gui/README.md))
- Static IR analyzer ([psm](../main/openroad/psm/README.md))
- Example tool ([exa](../main/openroad/exa/README.md))
- Physical design watermarking ([wmk](../main/openroad/wmk/README.md))

## Tool Checklist

Tools should make every attempt to minimize external dependencies.
Linking libraries other than those currently in use complicates the
builds and sacrifices the portability of OpenROAD. OpenROAD should be
portable to many different compiler/operating system versions and
dependencies make this vastly more complicated.

1. OpenROAD submodules reference tool `openroad` branch head. No git `develop`, `openroad_app`, or `openroad_build` branches.
1. Submodules used by more than one tool belong in `openroad/`, not duplicated in each tool repo.
1. Tool `BUILD` files do not set global compiler flags. Global flags live in `.bazelrc`; `OPENROAD_COPTS` in the top-level `BUILD.bazel` covers only the top-level targets. A tool may add per-target `copts` for a local need (for example, SWIG warnings).
1. No `main.cpp` or main procedure.
1. No compiler warnings for GCC or Clang with optimization enabled.
1. Does not call `flute::readLUT` (called once by `openroad`).
1. Tcl command(s) documented in top level `README.md` in flow order.
1. Command line tool documentation in tool README.
1. Conforms to Tcl command naming standards (no camel case).
1. Does not read configuration files. Use command arguments or support commands.
1. `.clang-format` at tool root directory to aid foreign programmers.
1. No `jenkins/`, `Jenkinsfile`, `Dockerfile` in tool directory.
1. Tests registered in `openroad/<tool>/test/BUILD` and run with `bazelisk test //openroad/<tool>/test/...`.
1. Test scripts use OpenROAD tcl commands (not `itcl`, not internal accessors).
1. Regressions report no memory errors with `valgrind` (stretch goal).
1. Regressions report no memory leaks with `valgrind` (difficult).
1. Ensure the top-level README and Tcl format are compliant.

## Code Linting and Formatting

OpenROAD uses both `clang-tidy` and `clang-format` to perform automatic linting and formatting whenever a pull request is submitted.

Run `clang-tidy` hermetically through Bazel. No Clang Tooling setup is
required, and the toolchain matches what CI will use:

```shell
# Single module:
bazelisk build --config=lint //openroad/<module>/...

# Full lint scope (excludes the upstream OpenSTA submodule):
bazelisk build --config=lint -- //openroad/... //third-party/... -//openroad/sta/...
```

Reports land under `bazel-bin/<pkg>/<target>_rules_lint/` as
`*.AspectRulesLintClangTidy.out` files. Generated files (SWIG, bison, flex)
and targets tagged `no-lint` are skipped automatically.

To run `clang-tidy` on all files, or on a subset with `--filter=REGEX`, use
`etc/run-clang-tidy.sh`. It runs `clang-tidy` in parallel and caches the
results, so subsequent runs only have to operate on changed files. It uses the
`clang-tidy` of the Bazel toolchain.

```shell
etc/run-clang-tidy.sh --filter='openroad/odb/'
```

Use the existing Bazel LLVM formatter to match CI's clang-format 23.1.2, even when the system formatter has a different version.
From the repository root, check its version:

```shell
bazelisk run @llvm//tools:clang-format -- --version
```

Use absolute file paths because Bazel can change the working directory:

```shell
bazelisk run @llvm//tools:clang-format -- -i "$PWD/source_file.cpp"
```

## Doxygen

OpenROAD uses Doxygen style comments to generate documentation.
See the generated documentation <a href="../doxygen_output/html/index.html">here</a>.
Our preferred syntax for Doxygen comments can be found in this
[file](../main/openroad/odb/include/odb/db.h). Also, do refer to the official Doxygen
documentation for more information on what you can include in your Doxygen
comments [here](https://www.doxygen.nl/manual/docblocks.html).

Below shows an example snippet taken from `./openroad/odb/include/odb/db.h`:

```cpp
///
/// dbProperty - Int property.
///
class dbIntProperty : public dbProperty
{
 public:
  /// Get the value of this property.
  int getValue();

  /// Set the value of this property.
  void setValue(int value);

  /// Create a int property. Returns nullptr if a property with the same name
  /// already exists.
  static dbIntProperty* create(dbObject* object, const char* name, int value);

  /// Find the named property of type int. Returns nullptr if the property does
  /// not exist.
  static dbIntProperty* find(dbObject* object, const char* name);
};
```

## Guidelines

1. Internally, the code should use `int` for all database units and `int64_t`
for all area calculations. Refer to this [link](DatabaseMath.md) for a more
detailed writeup on the reasons why this approach is preferred. The only
place that the database distance units should appear in any program
should be in the user interface, as microns are easier for humans than DBUs.

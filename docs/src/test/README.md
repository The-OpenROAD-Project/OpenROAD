# Manpages Test Framework

The documentation tests are split into two groups:

- **Static tests** — declared in `docs/src/test/BUILD`, they exercise the
  documentation *scripts* against fixed inputs checked in beside them.
- **Dynamic (per-module) tests** — generated for each documented module, they
  check that a module's `README.md` and `messages.txt` stay consistent with its
  Tcl sources.

All of them run in seconds. Only `man_func` needs the OpenROAD binary. See [Documentation Check Tests](../doc_check_tests.md) for the
Bazel-level view.

## Running the tests

```shell
# The static tests
bazelisk test //docs/src/test/...

# The per-module checks
bazelisk test --test_tag_filters=doc_check //src/...
```

## Static tests

These three are declared in `BUILD`:

#### Translator

The code file can be found [here](translator.py).
The objective of this test is to test if the underlying `README.md`
file can be converted to roff format via regex extraction. Namely,
the script checks for equality in the number of function names,
descriptors, synopsis, options and arguments detected per Tcl command.

It runs against the checked-in fixtures `translator.md` (a frozen module
README) and `translator.txt` (a frozen `messages.txt`), comparing the result to
`translator.ok`. Those fixtures are test input — do not edit them to fix a
problem in a real module README.

#### Man functionality check

The code file can be found [here](man_func.tcl).
The objective of this test is to check the functionality of the Tcl
`man` command implemented within the OpenROAD binary.
Mode 1 runs `man -manpath <dir> <CMD>` on the pages that `//docs:man_pages`
generates. Mode 2 runs `man <CMD>` without `-manpath`; a test binary has no
installed pages, so it checks the not-found error.

#### Bazel developer activities check

The code file can be found [here](bazel_developer_activities_check.py).
It asserts that the Bazel developer documentation still covers the key
day-to-day activities, and fails naming the topic that went missing.

## Dynamic (per-module) tests

For all the tests below, do make sure to update it locally every
time you make a change to the `README.md`, update messages, or
make a change to the top-level `Tcl` code.

Each documented module gets two tests, declared in its own
`src/{module}/test/BUILD` and tagged `doc_check`:

#### README-messages check

The name of this test is `{module}_readme_msgs_check`.
It validates that the module's `README.md` parses correctly into man2 format
and that its `messages.txt` parses into man3 format.

#### Man-to-Tcl check

The name of this test is `{module}_man_tcl_check`.
The objective of this test is to ensure that there are similar counts of
command in the following: `proc`, `help`, `man`.

`proc` and `help` commands are parsed from the Tcl file, whereas
`man` commands are parsed from the README file.

Note this compares **counts only** — a command renamed in the Tcl file and left
stale in the README still passes as long as the totals match.

## Not currently wired up

`man_tcl_params.py` performs per-command name and flag/key signature matching
across the Tcl `help` output, the `proc` definitions, and the README. It is
stricter than `man_tcl_check`, but it is in no Bazel target, so it does not
run today.

## New Test Checklist

Adding a new static test called `func`, you must create/update the following:
- `func.py|tcl`: Test script.
- `func.ok`: Log output of the test.
- `.*ok`: Ideal file output of the test (can be `def`, `lef` etc).
- `BUILD`: Add a `doc_check_test` (Python) or `regression_test` (Tcl) named
  `func`.

## Authors

Jack Luar (Advisor: Cho Moon)

## License

BSD 3-Clause License. See [LICENSE](../../../LICENSE) file.

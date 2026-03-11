# Documentation Check Tests

## Overview

OpenROAD has lightweight documentation tests that validate consistency
between source code, README files, and man pages. These tests are pure
Python and run in seconds without building the OpenROAD binary.

## Running the tests

```shell
# All documentation checks (~48 tests, seconds)
bazelisk test //:doc_test

# Duplicate message ID check
bazelisk test //:dup_id_test

# Both are also included in the full test suite
bazelisk test //src/...
```

## What gets tested

### Per-module tests (24 modules)

Each module has two documentation tests:

- **`{module}_readme_msgs_check`** — Validates that README.md parses
  correctly into man2 format and messages.txt parses into man3 format.

- **`{module}_man_tcl_check`** — Validates that command counts match
  across help strings, proc definitions, and README documentation.

### Repository-wide tests

- **`dup_id_test`** — Checks for duplicate logger message IDs across
  all source files using `etc/find_messages.py`.

## How it works

The tests use the `doc_check_test` Bazel macro defined in
`test/regression.bzl`. This is a lightweight variant of
`regression_test` that does not depend on `//:openroad`, so no C++
compilation is triggered.

Each module's `messages.txt` is generated on-demand by a Bazel
`genrule` that runs `etc/find_messages.py` over the module's source
files.

## Relationship to Jenkins

Jenkins runs "Documentation Checks" and "Find Duplicated Message IDs"
as stages in the PR pipeline. These Bazel tests cover the same checks.
Once this is validated in CI, the corresponding Jenkins stages can be
removed:

- Jenkins "Documentation Checks" stage → `bazelisk test //:doc_test`
- Jenkins "Find Duplicated Message IDs" stage → `bazelisk test //:dup_id_test`

## Adding a new module

When adding documentation tests for a new module:

1. Add `exports_files` and `messages_txt` genrule to `src/{module}/BUILD`
2. Add `doc_check_test` entries to `src/{module}/test/BUILD`
3. Add the test targets to the `doc_test` suite in `BUILD.bazel`

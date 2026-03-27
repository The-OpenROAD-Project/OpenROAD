# Documentation Check Tests

## Overview

OpenROAD has lightweight documentation tests that validate consistency
between source code, README files, and man pages. These tests are pure
Python and run in seconds without building the OpenROAD binary.

## Running the tests

```shell
# All documentation checks (~48 tests, seconds)
bazelisk test --test_tag_filters=doc_check //src/...

# Duplicate message ID check
bazelisk test //:dup_id_test
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
compilation is triggered. All doc check tests are automatically tagged
with `doc_check`, which allows tag-based discovery via
`--test_tag_filters=doc_check`.

Each module's `messages.txt` is generated on-demand by the
`messages_txt` macro (also in `test/regression.bzl`) that runs
`etc/find_messages.py` over the module's source files using the Bazel
Python toolchain.

## Relationship to Jenkins

Jenkins runs "Documentation Checks" and "Find Duplicated Message IDs"
as stages in the PR pipeline. These Bazel tests cover the same checks.
Once this is validated in CI, the corresponding Jenkins stages can be
removed:

- Jenkins "Documentation Checks" stage → `bazelisk test --test_tag_filters=doc_check //src/...`
- Jenkins "Find Duplicated Message IDs" stage → `bazelisk test //:dup_id_test`

## Adding a new module

When adding documentation tests for a new module:

1. Add `filegroup(name = "doc_files")` and `messages_txt()` macro call
   to `src/{module}/BUILD` (load `messages_txt` from
   `//test:regression.bzl`)
2. Add `doc_check_test` entries to `src/{module}/test/BUILD`

The `doc_check` tag is automatically added by the macro, so
`--test_tag_filters=doc_check` picks up new tests without any changes
to the root `BUILD.bazel`.

# Documentation Check Tests

## Overview

OpenROAD has lightweight documentation tests that validate consistency
between source code, README files, and man pages. These tests are pure
Python and run in seconds without building the OpenROAD binary.

## Running the tests

```shell
# All documentation checks (seconds)
bazelisk test --test_tag_filters=doc_check //openroad/...

# Duplicate message ID check
bazelisk test //:dup_id_test
```

## What gets tested

### Per-module tests

Each module has two documentation tests:

- **`{module}_readme_msgs_check`** — Validates that README.md parses
  correctly into man2 format and messages.txt parses into man3 format.

- **`{module}_man_tcl_check`** — Validates that command counts match
  across help strings, proc definitions, and README documentation.

### Repository-wide tests

- **`dup_id_test`** — Checks for duplicate logger message IDs across
  all source files using `etc/find_messages.py`.

## How it works

The README/messages tests use `readme_msgs_check_test` from
`bazel/readme_msgs_checker.bzl`. It supplies the shared checker and helpers
through Bazel runfiles, so modules do not need their own copies of the
checker. The existing regression harness still compares each check's log
against `{module}_readme_msgs_check.ok`.

The man/Tcl checks use `py_test`. Both test types are tagged with
`doc_check` and do not depend on `//:openroad`, so no C++ compilation is
triggered. `--test_tag_filters=doc_check` selects these tests.

Each module's `messages.txt` is generated on-demand by the
`messages_txt` macro (also in `test/regression.bzl`) that runs
`etc/find_messages.py` over the module's source files using the Bazel
Python toolchain.

## Relationship to Jenkins

Jenkins runs "Documentation Checks" and "Find Duplicated Message IDs"
as stages in the PR pipeline. These Bazel tests cover the same checks.
Once this is validated in CI, the corresponding Jenkins stages can be
removed:

- Jenkins "Documentation Checks" stage → `bazelisk test --test_tag_filters=doc_check //openroad/...`
- Jenkins "Find Duplicated Message IDs" stage → `bazelisk test //:dup_id_test`

## Adding a new module

When adding documentation tests for a new module:

1. Add `filegroup(name = "doc_files")` and `messages_txt()` macro call
   to `openroad/{module}/BUILD` (load `messages_txt` from
   `//test:regression.bzl`)
2. Add a `readme_msgs_check_test` entry to `openroad/{module}/test/BUILD`,
   loading it from `//bazel:readme_msgs_checker.bzl`. Pass the module's
   `doc_files` and `messages_txt` targets as `data`, and add the expected
   `{module}_readme_msgs_check.ok` log.
3. Add the module's `man_tcl_check` target as a `py_test` with the
   `doc_check` tag.

The `doc_check` tag is automatically added by `readme_msgs_check_test`, so
`--test_tag_filters=doc_check` picks up new tests without any changes
to the root `BUILD.bazel`.

---
name: fix-bug
description: >
  Fix an OpenROAD bug from a GitHub issue. Analyzes the issue, traces the
  root cause through the codebase, implements the fix, creates regression
  tests (registered in Bazel), runs clang-format, and
  prepares a signed-off commit. Use when given a bug report issue number,
  error code (e.g. GPL-0305, DRT-0001, RSZ-2007), or when asked to fix a
  bug in any OpenROAD module (ant, cts, dpl, drt, gpl, grt, mpl, odb,
  pad, pdn, psm, rcx, rsz, sta, stt, tap, upf, utl, etc.).
argument-hint: <issue-number-or-error-code>
---

# Fix OpenROAD Bug

You are fixing a bug related to **$ARGUMENTS**.

## 1. Understand the bug

If `$ARGUMENTS` is a GitHub issue number:
```bash
gh issue view $ARGUMENTS --repo The-OpenROAD-Project/OpenROAD \
  --json title,body,labels,comments
```

Extract from the issue:
- **Error string** (e.g. `GPL-0305`, `DRT-0001`) -- search for the exact message ID
- **Module** -- infer from labels or error prefix
- **Steps to reproduce** -- look for attached scripts, Tcl commands, or tarballs
- **Stack trace** -- if a crash, identify the faulting function

If `$ARGUMENTS` is an error code, search for it directly:
```bash
grep -rn "ERROR_CODE" src/MODULE/src/
```

## 2. Trace the root cause

Follow OpenROAD's "trace bugs upstream" principle: find where the bad
data is **created**, not where it is **reported**. The message ID
(`grep -rn "ERROR_ID" src/`) marks where it is reported; the bug is
often in a different function or file, wherever the offending value is
set.

Prefer a fix in OpenROAD code (e.g., `src/dbSta/`, `src/rsz/`) over one
in `src/sta/`. OpenSTA is a separate submodule: changing it takes a PR
in the OpenSTA repo and then a submodule update here, so ask before
modifying it, and do so only when the root cause is there and a fix on
the OpenROAD side would be a workaround.

## 3. Implement the fix

Follow the coding practices in `docs/agents/coding.md`. Use `int64_t`
for area calculations, and remove code made unreachable by `error()` or
`throw`.

## 4. Create regression test

Every bug fix needs a test that reproduces the exact failure mode from
the issue (same error code, same stack frame, same condition) so a
future regression is caught immediately.

Use the **add-test** skill (`.agents/skills/add-test/SKILL.md`) for the
full workflow: writing the test, generating golden files, and
registering in Bazel.

## 5. Format and commit

```bash
# Format C++ with CI's pinned clang-format (skips src/sta and *.i)
bazelisk run //:tidy_cc

# Stage and commit with DCO sign-off
git add <changed-files>
git commit -s -m "MODULE: fix BRIEF_DESCRIPTION

Fixes #ISSUE_NUMBER"
```

## 6. Verify

- [ ] Test passes: `bazelisk test --test_output=errors //src/MODULE/test:TEST_NAME-tcl_test`
- [ ] No unintended changes: `git diff HEAD`
- [ ] Test registered in Bazel `BUILD`
- [ ] No `src/sta/` changes, unless agreed with the user
- [ ] C++ files formatted with `bazelisk run //:tidy_cc`
- [ ] Commit is signed off (`-s`)

---
name: fix-bug
description: >
  Fix an OpenROAD bug from a GitHub issue. Analyzes the issue, traces the
  root cause through the codebase, implements the fix, creates regression
  tests (registered in both CMake and Bazel), runs clang-format, and
  prepares a signed-off commit. Use when given a bug report issue number,
  error code (e.g. GPL-0305, DRT-0001, RSZ-2007), or when asked to fix a
  bug in any OpenROAD module (ant, cts, dpl, drt, gpl, grt, mpl, odb,
  pad, pdn, psm, rcx, rsz, sta, stt, tap, upf, utl, etc.).
  Also triggers on: "fix issue", "debug this error", "resolve crash",
  "fix segfault", "repair", "patch bug".
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
data is **created**, not where it is **reported**.

1. Find the error message source:
   ```bash
   grep -rn "ERROR_ID" src/
   ```
2. Read the surrounding code to understand the condition that triggers it
3. Trace the data flow backward -- who sets the offending value?
4. Identify the actual bug location (often in a different function/file
   from where the error is reported)

**Critical constraint**: Never modify files under `src/sta/`. OpenSTA is
managed upstream. If the root cause is in OpenSTA, report this finding
instead of patching it.

## 3. Implement the fix

- Follow Google C++ Style Guide and OpenROAD coding practices
- Use modern C++20 features where appropriate
- Use `{...}` braces even for single-line statements
- Keep functions under 100 lines
- Use `int64_t` for area calculations to prevent integer overflow
- Add `const` qualifiers where possible
- Remove any unreachable code after `error()` or `throw`

Read `docs/contrib/CodingPractices.md` if unfamiliar with project idioms.

## 4. Create regression test

Every bug fix needs a test that reproduces the original bug and verifies
the fix. The test must be registered in **both CMake and Bazel** --
forgetting Bazel is the #1 mistake -- it silently passes locally but
fails in CI.

For the full workflow (writing the Tcl test, generating golden files,
dual registration, running with `./regression -R`), read:
- `docs/agents/testing.md` -- the project's testing guide
- `.agents/skills/add-test/SKILL.md` -- step-by-step add/register/verify procedure

The bug fix's test should reproduce the exact failure mode from the
issue (same error code, same stack frame, same condition) so a future
regression is caught immediately.

## 5. Format and commit

```bash
# Format changed C++ files (NEVER format src/sta/* or *.i files)
clang-format -i <changed-cpp-files>

# Stage and commit with DCO sign-off
git add <changed-files>
git commit -s -m "MODULE: fix BRIEF_DESCRIPTION

Fixes #ISSUE_NUMBER"
```

## 6. Verify

- [ ] Test passes: `cd src/MODULE/test && ./regression -R TEST_NAME`
- [ ] No unintended changes: `git diff HEAD`
- [ ] Test registered in both CMake and Bazel
- [ ] No `src/sta/` files modified
- [ ] C++ files formatted with clang-format
- [ ] Commit is signed off (`-s`)

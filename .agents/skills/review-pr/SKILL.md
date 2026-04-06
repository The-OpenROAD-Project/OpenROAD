---
name: review-pr
description: >
  Review an OpenROAD pull request following the project's review priority
  order: correctness > QoR impact > testing > architecture > style >
  process. Fetches PR diff, analyzes changes, and posts review comments.
  Use when asked to review a PR, check a pull request, look at changes,
  or provide code review for any OpenROAD module.
  Also triggers on: "review PR", "check this PR", "look at pull request",
  "code review", "review changes", "review #NUMBER".
argument-hint: <pr-number>
---

# Review OpenROAD Pull Request

You are reviewing PR **$ARGUMENTS**.

## 1. Fetch PR context

```bash
gh pr view $ARGUMENTS --repo The-OpenROAD-Project/OpenROAD \
  --json title,body,labels,files,additions,deletions,baseRefName

gh pr diff $ARGUMENTS --repo The-OpenROAD-Project/OpenROAD
```

Identify:
- **Modules affected** -- from file paths (e.g., `src/rsz/` = resizer)
- **Change type** -- bug fix, feature, refactor, test, docs
- **Scope** -- number of files and lines changed

## 2. Review by priority order

Follow CONTRIBUTING.md review priorities exactly. Spend most effort on
the top priorities and less on lower ones.

### Priority 1: Correctness (most important)

Flag any code that could silently produce wrong output. An explicit
error is always preferable to silently incorrect behavior.

Check for:
- **Iterator invalidation** -- modifying a container while iterating
- **Use-after-free** -- especially with ODB objects that may be deleted
- **Off-by-one errors** -- in loop bounds, coordinate calculations
- **Integer overflow** -- area calculations must use `int64_t`
- **Null pointer dereference** -- but don't add overly defensive checks
  for objects that cannot be null
- **Unreachable code** -- code after `error()` or `throw` should be removed
- **Suppressed tests** -- if a test is disabled, ask whether it hides a bug
- **`src/sta/` modifications** -- these files must NOT be modified
  (OpenSTA is managed upstream)

### Priority 2: QoR Impact

Any change affecting placement, routing, timing, or physical design:
- Ask: "Does this have a QoR impact?"
- QoR-affecting changes need validation on real designs, not just unit tests
- Be skeptical of improvements claimed from a single design

### Priority 3: Testing

- Does every code change have an accompanying test?
- Are tests registered in **both CMake and Bazel**?
- Do golden files look reasonable?
- Are test inputs minimal?

### Priority 4: Architecture

- OpenROAD is single-process, single-database
- Watch for memory cost in heavily-instantiated classes (`dbITerm`, etc.)
- Check ODB schema changes require a revision bump

### Priority 5: Style

Don't flag issues handled by `clang-format` or `clang-tidy`.
Only flag:
- Missing `const` qualifiers
- C-style casts (should use C++ casts)
- Missing braces on single-line statements
- Functions exceeding 100 lines

### Priority 6: Process

- DCO sign-off present?
- PR focused on one bug or feature?
- Style fixes in separate commits?
- References an open issue for non-trivial changes?

## 3. Write review comments

**Be concise.** One-word or one-sentence comments when the issue is
clear (e.g., "const", "unreachable after throw", "`int64_t` for area").

**Ask probing questions** for non-obvious issues:
- "Have you tested this on a real design?"
- "Could this hide a bug?"
- "What happens if this is null?"

**Don't generate PR summaries** unless asked.

## 4. Post the review

```bash
# For individual comments on specific lines:
gh api repos/The-OpenROAD-Project/OpenROAD/pulls/$ARGUMENTS/comments \
  -f body="COMMENT" -f path="FILE" -f line=LINE -f commit_id="COMMIT"

# For a general review comment:
gh pr review $ARGUMENTS --repo The-OpenROAD-Project/OpenROAD \
  --comment --body "REVIEW_BODY"
```

## Review style guide

- Lead with the **severity**: `bug:`, `nit:`, `question:`, `suggestion:`
- Group related issues rather than commenting on each line separately
- If the PR is clean, say so briefly: "LGTM -- correctness and testing
  look solid."
- Do not request changes for style issues covered by automated tools

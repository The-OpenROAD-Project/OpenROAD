---
name: review-pr
description: >
  Review an OpenROAD pull request following the project's review priority
  order: correctness > QoR impact > testing > architecture > style >
  process. Fetches PR diff, analyzes changes, and prints draft review
  notes for the human reviewer to inspect and post manually.
  Use when asked to review a PR, check a pull request, look at changes,
  or provide code review for any OpenROAD module.
  Also triggers on: "review PR", "check this PR", "look at pull request",
  "code review", "review changes", "review #NUMBER".
argument-hint: <pr-number-or-url>
---

# Review OpenROAD Pull Request

You are reviewing PR **$ARGUMENTS**.

`$ARGUMENTS` may be either:
- A bare PR number (e.g. `10062`)
- A full GitHub PR URL (e.g. `https://github.com/The-OpenROAD-Project/OpenROAD/pull/10062`)

**Normalize first.** Before running any `gh` command, decide what
`$ARGUMENTS` is and bind two shell variables:

- If `$ARGUMENTS` is a bare number: `PR=$ARGUMENTS` and `REPO=The-OpenROAD-Project/OpenROAD`.
- If `$ARGUMENTS` is a URL: extract the trailing number into `PR`, and
  extract `<owner>/<repo>` from the URL path into `REPO`. This matters
  because URLs from forks (or any non-canonical mirror) point at a
  different repo, and passing `--repo The-OpenROAD-Project/OpenROAD`
  with such a URL silently mismatches.

## 1. Fetch PR context

```bash
gh pr view "$PR" --repo "$REPO" \
  --json title,body,labels,files,additions,deletions,baseRefName

gh pr diff "$PR" --repo "$REPO"
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

- PR focused on one bug or feature?
- Style fixes in separate commits?
- References an open issue for non-trivial changes?

## 3. Draft the review locally (do not post to GitHub)

**Important:** This skill never posts comments to GitHub. AI-generated
review comments can be noisy or wrong, and posting them directly to a
public PR creates work for maintainers and erodes trust in human
review. The human reviewer must read your draft, edit it, and decide
what (if anything) to submit.

Print your draft review to the terminal for the human to inspect. Use
this format so it is easy to copy specific items into the GitHub UI:

```
PR #<num>: <title>
Modules: <list>

== Priority 1 (correctness) ==
- <file>:<line>  bug: <one-line description>
- <file>:<line>  question: <one-line question>

== Priority 2 (QoR) ==
- <file>:<line>  question: does this affect placement/routing/timing on real designs?

== Priority 3 (testing) ==
- <observation about test coverage, dual-registration, golden files>

== Priority 4-6 (architecture / style / process) ==
- <only if material -- skip what clang-format/clang-tidy already covers>

== Overall ==
- <one or two sentences: LGTM, needs changes, or questions before approval>
```

### Drafting style

**Be concise.** One-word or one-sentence items when the issue is clear
(e.g., "const", "unreachable after throw", "`int64_t` for area").

**Ask probing questions** for non-obvious issues rather than prescribing
fixes:
- "Have you tested this on a real design?"
- "Could this hide a bug?"
- "What happens if this is null?"

**Lead with severity**: `bug:`, `nit:`, `question:`, `suggestion:`.

**Group related issues** rather than listing each line separately.

**Don't generate PR summaries** unless asked.

**Do not flag** issues already handled by `clang-format` or `clang-tidy`.

If the PR is clean, say so briefly: "LGTM -- correctness and testing
look solid."

After printing the draft, stop. Do **not** call `gh pr review`,
`gh pr comment`, `gh api .../comments`, or any other command that
posts to the PR. The human will copy what they want into the GitHub
web UI.

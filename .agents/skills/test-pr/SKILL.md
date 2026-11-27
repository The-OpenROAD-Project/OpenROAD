---
name: test-pr
description: >
  Test an OpenROAD pull request for correctness: trace the assumptions
  the change relies on, build minimal corner-case inputs that could
  break them, run them against the PR and a baseline, and report what
  was reproduced. Use when asked to test a PR, find or reproduce a bug
  in a PR, or challenge a "no regressions" claim. For reading the diff
  without running anything, use review-pr.
argument-hint: <pr-number-or-url>
---

# Test OpenROAD Pull Request

You are testing PR **$ARGUMENTS**.

`$ARGUMENTS` is a bare PR number or a full GitHub PR URL. Bind `PR` and
`REPO` as in `review-pr` (`REPO=The-OpenROAD-Project/OpenROAD` for a
bare number; `<owner>/<repo>` from the URL otherwise).

The goal is to find inputs the PR handles wrongly: wrong results, valid
input rejected, invalid input accepted, broken invariants, or a
consequential runtime or memory regression. Running the existing suite
is not the goal; CI does that.

The steps below are not strictly linear. A run that contradicts a
hypothesis, or shows something unexpected, sends you back to step 2
or 3.

## 1. Fetch and check out

```bash
gh pr view "$PR" --repo "$REPO" \
  --json title,body,files,baseRefName,headRefOid
gh pr diff "$PR" --repo "$REPO"
```

If `git status` shows uncommitted changes to tracked files, stop and
ask. Otherwise record the current branch, then check out the PR and
update submodules.

Do not create extra clones or worktrees; a second Bazel workspace
rebuilds everything.

## 2. Understand what the change assumes

From the PR description and diff, work out:
- What inputs and states must the changed code support?
- What result is required, and what would count as a violation?
- Which invariants does the algorithm or optimization depend on, and
  does the code actually guarantee each one across the callers and
  modes that reach it?

Answer from the code, not from comments or names.

For a bound, feasibility check, or pruning rule, establish which way it
must be conservative. A check that rejects input is only correct if no
valid solution exists.

## 3. Form hypotheses

A hypothesis is useful when it predicts something a run can contradict:
a specific input, and the result or branch you expect from it.

Pick corner cases the code gives a reason to suspect, not a generic
checklist.

## 4. Build inputs

Use the smallest input that separates correct from incorrect behavior,
in whatever form fits the claim: LEF/DEF plus a Tcl driver, a C++ unit
test, a direct API call, or a larger design when the concern is runtime
or memory. Keep work files in the scratchpad directory, not in the
source tree. Reuse tech and cell files from `openroad/<module>/test/`
where possible.

Write the cases so they can be handed to someone else as they are:
- One script per finding, in one directory, with a copy of every input
  it reads (netlists, LEF, liberty) next to it. Resolve paths from the
  script's own location; no path into this checkout, the home
  directory, or the scratchpad.
- Each script prints its own PASS or FAIL against the expected result.
- An input that exists only on the PR branch is copied in before any
  revision switch.
- A short README or runner script says how to run them.

When the diff makes an operation asymptotically worse, or adds work
that grows with design size in a way that looks costly, add a scaled
input and time that operation on the PR and the base at two or three
sizes. Do not skip the measurement because the base has the same
complexity; a larger constant is also a regression.

Choose an oracle that does not repeat the calculation under test: a
hand-derived witness, exhaustive enumeration on a tiny case, the base
revision, or an independent checker (for example `check_placement`,
`check_antennas`, `report_checks`, or a DRC count). A test that asserts
the new error message appears shows the check fires, not that the
rejection is correct.

## 5. Run

Build the PR and run the cases. Say so before starting a long build.

Run every case under a memory limit and a timeout (for example
`ulimit -v` and `timeout`), so a case that loops or allocates without
bound cannot take the machine down.

Build the merge base as well whenever a case fails or looks wrong on
the PR: a failure is a regression if the base handles the same input
correctly, or fails it in a better way. Skip the base build when it
could not prove anything (for example, the PR adds a command the base
does not have). Copy each binary out of `bazel-bin` before switching
revisions, and run `git submodule update --init --recursive` after each
switch.

- If the base fails the same way, it is not a regression. Find out
  whether the input is invalid or the bug predates the PR.
- Compare how each revision fails, not only whether it does. An error
  is the correct behavior over a silent wrong result, so a case where
  the base stops with an error and the PR completes with a wrong result
  is a regression.
- For a suspected wrong rejection, the control must complete and its
  output must pass the oracle. Reaching a different failure proves
  nothing.
- A diagnostic edit to the source (for example, bypassing a guard) must
  be reverted before any further comparison, and a binary built with it
  is not "the PR".

If something blocks execution, say what, what you tried, and which
cases were not run.

## 6. Tell the user what you found

Explain the findings in chat, in prose, as you would to a colleague.
Never post to the PR, and do not implement a fix.

For each finding, make clear where in the code it is, which assumption
fails, what input triggers it, and what the PR and the base each did.
Say whether it was **reproduced** (actually run), **derived from the
code only**, or **unresolved**, and keep those apart. Mention what you
ruled out and what you did not get to.

If nothing survives, say so. Do not substitute style nits.

### Review comment

When at least one finding was reproduced, also give a review comment
the user can paste into the PR, in a fenced Markdown block, with these
sections in this order:

1. One or two sentences: what was built (PR and merge-base commit) and
   the overall result.
2. **What I checked and found correct**: the cases that passed and the
   oracle they passed against.
3. One numbered section per finding, titled with the behavior and
   marked "(regression)" or "(predates this PR)". Each names the
   testcase file, says what the PR did and what the base did, and
   explains the cause in the code. A runtime finding gives a table of
   sizes against base and PR times.
4. **Running the testcases**: the exact commands for the bundle,
   including any memory limit a case needs.
5. **Not covered**: what was not tested.

State findings and causes only; do not suggest fixes in the comment.

### Testcase bundle

Pack the testcase directory from step 4 into one archive to go with
the comment. Before handing it over, check that it is safe to share,
and report the result of each check:

- No private data: search every file and the archive's own metadata
  for user names, email addresses, company or project names, and local
  paths. Pack with neutral ownership and timestamps (for example
  `tar --owner=0 --group=0 --numeric-owner`).
- Every bundled input is public (tracked in the public repository, or
  written for this test). Note the license of third-party files such as
  cell libraries.
- Scripts only read their own directory and run the binary they are
  given. No network access, and no writes outside the directory. Say
  which case needs a memory limit and why.

If the bundle contains private data, or a case only reproduces with
private data (a customer design, a proprietary PDK, an internal path),
tell the user so prominently, at the top of the reply and not in a
footnote, and do not present the bundle as ready to share.

Finish by returning to the branch recorded in step 1 and updating
submodules. Leave the reproduction files in the scratchpad and say
where they are. If a case is worth keeping as a regression test, say so
and point to `add-test`; do not add it to the PR branch.

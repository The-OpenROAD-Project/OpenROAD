# Bazel Developer Activities and `.bzl` Training Guide

This guide maps common OpenROAD developer activities to Bazel workflows and provides practical training steps for reading and maintaining `.bzl` (Starlark) files.

It is intended for contributors who already build and test OpenROAD, and want to become productive with Bazel internals and review-ready changes.

## Scope

This document covers:

- day-to-day developer activities
- where to find relevant Bazel logic in the repository
- how to approach `.bzl` and Starlark changes safely
- minimal commands to verify changes locally before opening a PR

For general Bazel usage (running tests, build configs, caching, ORFS details), see:

- [Testing local changes with Bazel](Bazel.md)
- [Bazel targets](Bazel-targets.md)
- [Bazel Developer Guide: Caching](Bazel-caching.md)
- [Handling breaking changes between OpenROAD and ORFS](Bazel-breaking-changes.md)

---

## Repository areas to know

You will work mostly in:

- `BUILD.bazel` / `BUILD` files: target declarations
- `MODULE.bazel` and `MODULE.bazel.lock`: dependency/module management
- `.bazelrc`: shared Bazel flags and configs
- `bazel/*.bzl`: shared Starlark macros and helpers used across targets
- `test/regression.bzl` and `test/orfs/**/*.bzl`: regression and ORFS-related Starlark logic

Treat `.bzl` files as code: keep changes small, reviewable, and tested.

---

## Activity matrix (what developers do most often)

## 1) Modify C++/Tcl/Python code and run fast local checks

Typical flow:

1. make code change
2. run focused tests in affected subtree
3. run broader checks before PR

Examples:

    bazelisk test //src/<tool>/...

When broad integration confidence is needed:

    bazelisk test //...

Use narrow scope first; widen only as confidence increases.

## 2) Add or update a target in a `BUILD` file

Common tasks:

- add a new test target
- add data dependencies
- split/reuse libraries for better build graph hygiene

Recommended process:

1. copy a nearby target pattern
2. keep visibility minimal
3. avoid over-exporting internals
4. run only the new/changed target first
5. then run subtree tests

Example validation steps:

    bazelisk query 'kind(test, //src/<tool>/...)'
    bazelisk test //src/<tool>/test:<new_test_target>

The test target name is derived from the test file name with a suffix added
based on type: Tcl tests get `_tcl` and Python tests get `_py`. For example,
`src/grt/test/fastroute.tcl` becomes target `fastroute_tcl`, so you would run:

    bazelisk test //src/grt/test:fastroute_tcl

Use the `bazelisk query` command above to list all test targets and find the
exact name for any test in the subtree.

## 3) Change shared build logic in `.bzl`

This has wider blast radius than editing one `BUILD` file.

Safe process:

1. isolate the macro/rule behavior you need to change
2. preserve backward-compatible call patterns where possible
3. test one direct consumer target first
4. test additional representative consumers
5. run a broader sweep before PR

Suggested progression:

- one directly affected target
- one unrelated consumer (sanity against regressions)
- affected subtree tests
- optional full `src/...` or `...` based on change scope

## 4) Update ORFS/Bazel-ORFS coupling

If your change touches integration boundaries:

- verify OpenROAD side and ORFS assumptions together
- use the documented ORFS debug/issue-generation workflow in `test/orfs/README.md`
- run at least affected ORFS smoke tests before PR

## 5) Update Bazel module/dependency state

When bumping module inputs:

- prefer project-documented bump flow
- review both `MODULE.bazel` and lock updates
- keep diffs minimal and intentional
- verify representative build/tests after update

---

## `.bzl` training path (practical, repo-oriented)

## Step 1: Read Starlark with intent

When reading a `.bzl` file, identify:

- exported symbols (macros/rules) used by `BUILD` files
- inputs (attrs/kwargs) and defaults
- what target(s) the macro expands into
- implicit outputs, tags, and toolchain assumptions

Focus first on call sites in nearby `BUILD` files.

## Step 2: Learn by tracing one macro end-to-end

Pick one macro used in your area and trace:

1. `BUILD` call site
2. macro parameters and normalization
3. final native rules emitted
4. resulting target graph via query

This builds intuition faster than reading Starlark in isolation.

## Step 3: Make no-op/refactor-safe edits first

Before behavior changes, practice with small, review-safe edits:

- variable naming cleanup
- comments clarifying non-obvious behavior
- extracting small helper functions without semantic changes

Then run representative tests. This gives confidence in your local loop and reduces risk.

## Step 4: Introduce behavior changes behind clear conditions

For real logic changes:

- keep old path available when practical
- gate new behavior with explicit kwargs or clear branch conditions
- avoid hidden behavior changes to unrelated consumers

## Step 5: Validate like a maintainer

For `.bzl` changes, do not stop at one passing target.
Validate across:

- direct consumer(s)
- at least one unrelated consumer
- broader subtree or project-level tests proportional to blast radius

---

## Code review checklist for Bazel / `.bzl` changes

Before opening a PR, verify:

- [ ] change is minimal and scoped to one intent
- [ ] `BUILD`/`.bzl` naming is clear and consistent with nearby code
- [ ] no accidental visibility broadening
- [ ] test target selection matches changed area
- [ ] representative tests passed locally
- [ ] docs updated when behavior/workflow changed

For larger `.bzl` changes, include a short “impact summary” in PR description:

- what changed
- who consumes it
- what was tested

---

## Common pitfalls and how to avoid them

- **Changing shared macros without representative tests**  
  Always test more than one consumer.

- **Overusing broad target patterns too early**  
  Start narrow (`//src/<tool>/...`), then widen.

- **Conflating host/target configs when debugging**  
  Use guidance in [Bazel targets](Bazel-targets.md) and [Bazel.md](Bazel.md).

- **Mixing unrelated cleanup with behavioral changes**  
  Separate commits improve review quality and rollback safety.

- **Unclear PR validation evidence**  
  Include exact commands you ran and the scope they cover.

---

## Suggested onboarding exercises for new developers

These are low-risk training tasks that build Bazel confidence:

1. run and list tests in a single tool subtree
2. add a tiny test target in a local branch and validate it
3. trace one `.bzl` macro from call site to emitted rules
4. make a non-functional clarity edit in `.bzl` and confirm no regressions
5. document your local validation commands in PR style

---

## Minimal verification template (copy/paste for PR notes)

Use and adapt this in your PR description:

    Scope:
    - <what changed>

    Validation:
    - bazelisk test //src/<tool>/test:<specific_target>
    - bazelisk test //src/<tool>/...
    - bazelisk test //src/...

    Notes:
    - <any known limitations or follow-up work>

Keep validation proportional to change scope.

---

## When to ask for help

Ask early if:

- you are unsure whether a `.bzl` macro is shared across many consumers
- a change requires module/toolchain decisions beyond local context
- target/config behavior is surprising across `build` vs `test`

A short design note in the PR discussion is better than a large speculative change.

---

## Summary

For OpenROAD Bazel work, reliability comes from:

- small scoped edits
- understanding call site → macro → emitted rule flow
- representative validation across consumers
- explicit, reproducible test evidence in PRs

If you follow this workflow, your `.bzl` and Bazel changes are much more likely to be easy to review and safe to merge.

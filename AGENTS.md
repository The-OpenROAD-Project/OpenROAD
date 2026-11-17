# OpenROAD - AI agent context

## Quick Reference

Detailed guides are in `docs/agents/` subdirectory:

- **Coding Patterns** (loaded): @docs/agents/coding.md
- **Testing Guide**: See `docs/agents/testing.md`
- **Test Strategy** (where tests live, DB/STA dependencies): See `docs/agents/testing-strategy.md`
- **Git & CI**: See `docs/agents/ci.md`

## Critical Rules

1. **Ask before modifying `src/sta/` files** -- OpenSTA (`Sdc.cc`, `Power.cc`, `Sdc.tcl`, etc.) is a separate submodule: a change there takes a PR in the OpenSTA repo and then a submodule update here. Prefer fixes in OpenROAD code (e.g., `src/dbSta/`, `src/rsz/`) unless there is a good reason to change OpenSTA.
2. Format C++ before commit with `bazelisk run //:tidy_cc`, which uses CI's pinned clang-format and skips `src/sta`, `*.i`, generated ODB code, and `third-party/`. Don't use a system `clang-format`; its version can format differently from CI's.
3. **Always use `git commit -s`** for DCO compliance. Do not review DCO compliance; CI handles it.
4. After a merge or pull, run `git submodule update --init --recursive`. After committing or amending inside the `src/sta` submodule, `git add src/sta` in the parent and commit.

## Build and Test

Bazel is the only build system, on Linux and macOS.
Register new source files, tools, and tests in Bazel `BUILD` files.

- Build: `bazelisk build //:openroad` (the binary is `bazel-bin/openroad`). `etc/Build.sh` is the release build-and-install script (ORFS, Docker, and packaging), not for everyday builds.
- Test one module: `bazelisk test //src/<module>/test/...`
- Test one Tcl test: `bazelisk test --test_output=errors //src/<module>/test:<name>-tcl_test`
- Lint/format check for all languages (C++, Tcl, Bazel, Python, and YAML): `bazelisk test //:lint_test`; auto-fix with `bazelisk run //:fix_lint`

## AI Agent Skills

Skills are located in `.agents/skills/` (with `.claude/skills` symlink for Claude Code).

| Skill | Purpose | Invocation |
|-------|---------|------------|
| `triage-issue` | Reproduce bug and minimize test case with whittle.py | `/triage-issue <issue#>` |
| `fix-bug` | Trace root cause, implement fix, create tests, prepare commit | `/fix-bug <issue#-or-error-code>` |
| `add-test` | Add integration/unit tests registered in Bazel | `/add-test <module> [description]` |
| `review-pr` | Draft local PR review notes (correctness > QoR > testing); human posts | `/review-pr <pr#-or-url>` |
| `run-web-viewer` | Launch the web viewer on a design and drive it in headless Chrome; screenshots | `/run-web-viewer` |

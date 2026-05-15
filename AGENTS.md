# OpenROAD - AI agent context

This file provides project-specific guidance for AI agent sessions working on the OpenROAD codebase.

## Quick Reference

Detailed guides are in `docs/agents/` subdirectory:

- **Build Pitfalls**: See `docs/agents/build.md`
- **Testing Guide**: See `docs/agents/testing.md`
- **Git & CI**: See `docs/agents/ci.md`
- **Coding Patterns**: See `docs/agents/coding.md`

## Critical Rules

1. **Ask before modifying `src/sta/` files** -- OpenSTA is managed upstream (`Sdc.cc`, `Power.cc`, `Sdc.tcl`, etc.). Prefer fixes in OpenROAD code (e.g., src/dbSta/, src/rsz/) when possible. 
2. Run `clang-format -i <files>` for C++ files before commit. **NEVER** for `src/sta/*` and `*.i` files.
3. **Always use `git commit -s`** for DCO compliance.
4. When amending submodule commits, parent repo submodule reference must also be updated via `git submodule update --init --recursive`. It is needed after any merge/pull.
5. **Trace bugs upstream** -- when a bug appears in output (e.g., Verilog), find the data creation point (e.g., `buffer_ports`, `remove_buffers`), not the serialization point (e.g., `VerilogWriter`).

## AI Agent Skills

Skills are located in `.agents/skills/` (with `.claude/skills` symlink for Claude Code).

| Skill | Purpose | Invocation |
|-------|---------|------------|
| `triage-issue` | Reproduce bug and minimize test case with whittle.py | `/triage-issue <issue#>` |
| `fix-bug` | Trace root cause, implement fix, create tests, prepare commit | `/fix-bug <issue#-or-error-code>` |
| `add-test` | Add integration/unit tests with dual CMake+Bazel registration | `/add-test <module> [description]` |
| `review-pr` | Draft local PR review notes (correctness > QoR > testing); human posts | `/review-pr <pr#-or-url>` |

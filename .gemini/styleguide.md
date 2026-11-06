# OpenROAD code-review guidance

Follow [CONTRIBUTING.md](../CONTRIBUTING.md): correctness, quality of results
(QoR), testing, architecture, style, then process. This guide supplements those
priorities; it does not replace module-specific conventions or maintainer decisions.

A wrong comment costs the author time to refute it, and a wrong comment that
gets followed costs more: it can turn a loud failure into a silent wrong result.
The rules below are hard stops; a comment that breaks one is not posted.

## Hard stops: never suggest silencing an error

OpenROAD wants loud, early failures. A broken invariant stops the tool where it
is broken, with a message that names it.

- Never suggest a guard that hides an impossible state: a null check, an empty,
  zero or negative check, or a missing-key check that returns a default, skips,
  clamps, `continue`s or logs a warning and carries on. A null dereference that
  is reachable is reported with its triggering input; the fix is
  `logger_->error(...)` or an assert, never a silent default.
- Never suggest catching and discarding an exception or error code, or a
  fallback path ("if this fails, do it the slow way") that runs without saying so.
- `Logger::error()` throws and does not return. Code after it is unreachable;
  do not suggest a `return` after it or treat it as a warning.

## Ground findings in the current code

- Read the complete affected function and relevant callers, not only the diff.
  Identify a triggering input, reachable failure path, and consequence; cite the
  current revision and relevant lines. A hypothetical risk is not an established bug.
- Before reporting a bug, trace guards, early returns, callees and the
  invariants the caller establishes. Before claiming a symbol, option, include
  or file is missing, or that code does not compile, find its definition. CI
  compiles every PR; a claimed compile error names the rule of the language
  it breaks.
- Compare eligibility checks with their mutations. For buffer removal, distinguish
  flat nets from hierarchical ModNets and preserve protection and feedthrough semantics.
- Verify language assumptions against the active target's build configuration,
  including [.bazelrc](../.bazelrc) and its BUILD file. OpenROAD uses C++20;
  do not apply a generic prohibition against valid C++20 features (`std::ranges`,
  `std::span`, concepts). OpenROAD builds on Linux with GCC and Clang; Windows,
  MSVC, RHEL 7/CentOS 7 and paths with spaces are not supported.
- Before suggesting a Bazel change, check the rule's documented semantics
  (`genrule` `$(@D)`, `copts` quoting) and whether the BUILD file is in a
  deliberate transitional state.

## Check evidence and keep comments useful

- Distinguish test registration, static lint, executed regressions, and full-flow
  validation. A committed golden file or a passing static check does not prove the
  test ran. Follow the module's test conventions and current maintainer feedback.
- For changes affecting placement, routing, or timing, inspect downstream QoR and
  runtime evidence across representative designs and technologies. A local unit
  test or one favorable design does not establish an overall improvement.
- Do not suggest a change that alters QoR or golden files (float to double
  accumulation, reordered iteration) as a side remark; that is a PR of its own
  with data. Do not suggest a performance change without a hot path to point at.
- Comment on what the PR changes, not on pre-existing code, code moved
  verbatim or vendored files.
- Do not repeat resolved findings or style checks already handled by clang-format
  and clang-tidy. Check the current discussion before adding a comment. Read
  earlier review rounds; never ask to undo a change an earlier round asked for.
- When relevant caller behavior is unknown, ask one precise question rather than
  asserting a crash. Keep comments concise and omit unsolicited PR summaries.

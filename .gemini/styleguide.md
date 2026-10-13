# OpenROAD code-review guidance

Follow [CONTRIBUTING.md](../CONTRIBUTING.md): correctness, quality of results
(QoR), testing, architecture, style, then process. This guide supplements those
priorities; it does not replace module-specific conventions or maintainer decisions.

## Ground findings in the current code

- Read the complete affected function and relevant callers, not only the diff.
  Identify a triggering input, reachable failure path, and consequence; cite the
  current revision and relevant lines. A hypothetical risk is not an established bug.
- Before reporting a null dereference, trace guards, early returns, and callees.
  A null value can represent an intentionally supported no-merge or no-op case.
  Rejecting it unconditionally can remove useful behavior.
- Compare eligibility checks with their mutations. For buffer removal, distinguish
  flat nets from hierarchical ModNets and preserve protection and feedthrough semantics.
- Verify language assumptions against the active target's build configuration,
  including [.bazelrc](../.bazelrc) and its BUILD file. OpenROAD uses C++20;
  do not apply a generic prohibition against valid C++20 features.

## Check evidence and keep comments useful

- Distinguish test registration, static lint, executed regressions, and full-flow
  validation. A committed golden file or a passing static check does not prove the
  test ran. Follow the module's test conventions and current maintainer feedback.
- For changes affecting placement, routing, or timing, inspect downstream QoR and
  runtime evidence across representative designs and technologies. A local unit
  test or one favorable design does not establish an overall improvement.
- Do not repeat resolved findings or style checks already handled by clang-format
  and clang-tidy. Check the current discussion before adding a comment.
- When relevant caller behavior is unknown, ask one precise question rather than
  asserting a crash. Keep comments concise and omit unsolicited PR summaries.

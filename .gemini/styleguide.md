# OpenROAD code-review guidance

Follow [CONTRIBUTING.md](../CONTRIBUTING.md): prioritize correctness, physical-design quality of results (QoR), and tests over style.

## Check a concrete execution path before reporting a bug

- For every correctness finding, identify the triggering input or state, the reachable code path, and the user-visible consequence. Cite the relevant lines; do not turn a hypothetical risk into a high-severity finding without evidence.
- Before claiming a null dereference, trace surrounding guards, early returns, and any called functions. A null value can be an intentional no-merge or no-op case; a new rejection may itself be a regression.
- When an operation has a read-only eligibility check and a separate mutation (for example, `canRemoveBuffer` and `removeBuffer`), verify they use the same semantics. Include exceptional paths such as hierarchy and disconnected terminals.
- Check language and build assumptions against the repository. OpenROAD's root `CMakeLists.txt` requires C++20; do not flag valid C++20 as unsupported.

## Keep reviews actionable

- Focus on correctness, placement/routing/timing QoR impact, missing regression tests, and unsafe database changes. Do not duplicate `clang-format` or `clang-tidy` style checks.
- For QoR claims, request evidence from representative designs and technologies, not a single favorable case. This does not apply to documentation-only changes.
- If a suspected problem depends on unknown caller behavior or unspecified input, ask one precise question or suggest a targeted test instead of asserting a crash.
- Keep comments concise and specific; skip automatic PR summaries, which are disabled in this repository.

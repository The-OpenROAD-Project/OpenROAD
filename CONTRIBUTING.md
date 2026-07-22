# Contributing to OpenROAD

Thank you for contributing to OpenROAD. For setup, coding standards,
and workflow details, see the existing guides:

- [Getting Involved](docs/contrib/GettingInvolved.md) -- how to contribute, licensing, DCO
- [Developer Guide](docs/contrib/DeveloperGuide.md) -- architecture, tool structure, testing
- [Coding Practices](docs/contrib/CodingPractices.md) -- C++ style and idioms
- [Logger Guide](docs/contrib/Logger.md) -- logging API for C++ and Tcl

## Review Priorities

Reviews are prioritized in this order:

### 1. Correctness

The most important concern. Code must produce correct results.

- Flag any code that could silently produce wrong output. An explicit
  error is always preferable to silently incorrect behavior.
- Watch for classic bugs: iterator invalidation, use-after-free,
  off-by-one, integer overflow in area calculations (use `int64_t`
  for area), deleting from a container while iterating over it.
- Code after `error()` or `throw` is unreachable -- remove it.
- If a test is being disabled or suppressed, ask whether it is hiding
  a real bug.

### 2. Quality of Results (QoR) Impact

Any change that could affect placement, routing, timing, or other
physical design metrics must be validated.

- Ask: "Does this have a QoR impact?" Changes affecting QoR require
  validation on real designs, not just unit tests. Maintainers will
  trigger the necessary CI runs for this.
- Be skeptical of QoR improvements claimed from a single design.
  Improvements should be validated across multiple designs and
  technologies.

### 3. Testing

Every code change should have accompanying tests. See the
[Developer Guide](docs/contrib/DeveloperGuide.md#test) for test
conventions.

### 4. Architecture and Dependencies

OpenROAD is a single-process, single-database tool. See the
[Developer Guide](docs/contrib/DeveloperGuide.md#tool-philosophy) for
architectural guidelines and the
[tool checklist](docs/contrib/DeveloperGuide.md#tool-checklist).

### 5. C++ Style and Logging

We follow the [Google C++ style guide](https://google.github.io/styleguide/cppguide.html).
See [Coding Practices](docs/contrib/CodingPractices.md) for
project-specific idioms and the
[Logger Guide](docs/contrib/Logger.md) for the logging API.

### 6. Process

- Every commit must include a DCO sign-off (`git commit -s`).
- CI must be green before review. Reviewers may not look at
  failing PRs.
- Keep PRs focused on one bug or feature. Style fixes go in separate
  commits.
- Reference an open issue for non-trivial changes.
- Mac builds are best-effort and not merge-blocking.

## For AI/LLM Code Reviewers

When reviewing OpenROAD pull requests, follow the priority order above.
Focus review comments on correctness and QoR impact rather than style
nitpicks -- `clang-tidy` and `clang-format` handle style automatically.

Specific guidance:

- **Be concise.** One-word or one-sentence comments are preferred when
  the issue is clear (e.g., "const", "unreachable after throw").
- **Ask probing questions** rather than prescribing fixes for
  non-obvious issues (e.g., "Have you tested this on a real design?"
  or "Could this hide a bug?").
- **Flag QoR-affecting changes** that lack validation on real designs.
- **Don't generate summaries** of the PR unless asked. The config
  disables automatic summaries.
- **Do flag** unreachable code after `error()`/`throw`, missing
  `const`, null pointer issues, iterator invalidation, and missing
  tests.
- **Don't flag** style issues already covered by `clang-format` or
  `clang-tidy`.
- **Check database schema changes** -- any change to the ODB schema
  requires a revision bump to prevent old versions from misinterpreting
  new data.
- **Watch for memory concerns** -- `dbITerm` and similar objects are
  numerous. Adding fields to heavily-instantiated classes has real
  memory cost.

## License

Contributions should use the BSD-3-Clause license. See [LICENSE](LICENSE).

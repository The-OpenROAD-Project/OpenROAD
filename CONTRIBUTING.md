# Contributing to OpenROAD

Thank you for contributing to OpenROAD. This document provides guidelines
for contributors and for AI/LLM code review tools assisting with pull
request reviews.

For detailed setup and workflow instructions, see the
[Developer Guide](docs/contrib/DeveloperGuide.md),
[Coding Practices](docs/contrib/CodingPractices.md), and
[Getting Involved](docs/contrib/GettingInvolved.md).

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
  secure-CI runs on real designs, not just unit tests.
- Be skeptical of QoR improvements claimed from a single design.
  Improvements should be validated across multiple designs and
  technologies.

### 3. Testing

Every code change should have accompanying tests.

- No database (`.db`) files in tests. Read LEF/DEF/Verilog to
  create the database.
- Regression tests must be runnable from any directory and must
  not leave the source tree dirty.
- Regression output should be concise -- not thousands of lines.
- Tests should pass `-no_init` to `openroad`.
- New features without tests are unlikely to be accepted.

### 4. Architecture and Dependencies

OpenROAD is a single-process, single-database tool. Respect this
architecture.

- All tools reside in one process with one database. File-based
  communication between tools and forking processes is strongly
  discouraged.
- Do not add runtime or build dependencies without serious
  justification. Consider copying a small relevant subset rather than
  taking a full library dependency.
- Code should be in the right module. Low-level utilities (`utl`) should
  not depend on higher-level tools. Consider where code naturally
  belongs in the dependency graph.
- Tool state belongs in classes, not global variables. Global variables
  prevent multi-threading and multiple tool instances.
- Submodules for new code are strongly discouraged.

### 5. C++ Style

We follow the [Google C++ style guide](https://google.github.io/styleguide/cppguide.html).
Key project-specific practices:

- Don't comment out code -- remove it. Git has history.
- Don't use prefixes on class/function/file names. Use namespaces.
- Namespaces should be short, lowercase (e.g., `drt`, `gpl`, not
  `TritonCTS`).
- No global variables. All state in classes.
- No magic numbers. Use named constants with documented units.
- No `continue` -- wrap the body in an `if` instead.
- No `== true` / `== false`. Use the boolean directly.
- No nested `if` -- use `&&`.
- Use `nullptr` not `NULL`, `std::vector` not C arrays, `new` not
  `malloc`, `std::abs` not C `abs`.
- Use `#pragma once` not `#ifndef` guards.
- Use `""` for project headers, `<>` only for system headers.
- Use range-based for loops, not iterator loops.
- Use forward declarations instead of includes where possible.
- Don't use `using namespace std`. Prefer `using namespace::symbol`.
- Break long functions into smaller ones that fit on one screen.
- Don't duplicate code fragments -- write functions.
- Run `clang-format` on all changed C++ files before committing.

### 6. User-Facing Quality

- Log messages must use the OpenROAD logger (`utl::info`, `utl::warn`,
  `utl::error`), not `printf` or `std::cout`. Use `utl::report`
  instead of `puts`.
- Messages start with a capital letter and end with a period.
- Error and warning messages should be actionable -- tell the user
  what to do, not just what went wrong.
- Don't abbreviate English words in messages (`Number` not `Num`,
  `Total` not `Tot`).
- Tcl commands use `snake_case`, not `camelCase`.
- User-visible commands go in the global namespace. Internal commands
  stay in the tool namespace.
- Tools should use Tcl commands exclusively for control -- no external
  configuration files.
- All database units are `int` internally. Microns only appear in the
  user interface.

### 7. Process

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
- **Flag QoR-affecting changes** that lack secure-CI validation.
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

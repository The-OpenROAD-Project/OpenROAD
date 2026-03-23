# Bazel Lint Targets

Bazel lint targets provide a single entry point for running linters and
auto-formatters locally, with all tool versions managed by Bazel — no manual
installs needed.

## Why Bazel for linting?

Managing linter dependencies is a surprisingly painful part of development.
Each tool has its own version, its own install method (`pip`, `npm`, `go
install`, distro packages), and its own set of transitive dependencies that
can conflict with each other or with the project. CI scripts paper over this
with ad-hoc `pip install` steps pinned in YAML files that drift out of sync,
break silently, and can't be tested locally without replicating the exact CI
environment.

Bazel solves this by treating linter tools as hermetic dependencies — the same
way it treats compilers and libraries. Tool versions are pinned in
`bazel/requirements.in` (for pip packages) or `MODULE.bazel` (for Bazel
modules), lock files ensure reproducibility, and `bazelisk test //:lint_test`
works identically on every developer's machine and in CI. No virtualenvs, no
"works on my machine", no guessing which version of `tclint` CI is running.

## Quick start

```bash
# Run all lint checks (~0.3s, no C++ build)
bazelisk test //:lint_test

# Auto-fix then lint
bazelisk run //:fix_lint
```

## Target naming convention

The `_test` suffix is a Bazel convention: `bazelisk test` discovers and runs
targets whose name ends in `_test`. `//:lint_test` is the top-level test suite
that aggregates all per-language lint checks — running `bazelisk test //:lint_test`
checks every language supported by this framework.

Targets follow a consistent `{verb}_{language}` pattern so that adding a new
language is mechanical and the naming is predictable:

| Pattern | Scope | Purpose |
|---------|-------|---------|
| `//:fix_lint` | all languages | Umbrella: auto-fix then lint everything |
| `//:lint_test` | all languages | Umbrella: run all lint checks (read-only) |
| `//:lint_{lang}_test` | one language | Lint check (read-only) |
| `//:fmt_{lang}_test` | one language | Format check (read-only) |
| `//:tidy_{lang}` | one language | Auto-format only |

Adding a new language means adding per-language targets and wiring them into
the two umbrella targets.

## Available targets

| Target | Type | What it does |
|--------|------|-------------|
| `//:lint_test` | `test_suite` | Umbrella: runs all lint checks |
| `//:fix_lint` | `sh_binary` | Umbrella: auto-fixes what can be fixed, then lints |
| `//:lint_tcl_test` | `sh_test` | Runs `tclint .` (lint rules) |
| `//:fmt_tcl_test` | `sh_test` | Runs `tclfmt --check .` (formatting) |
| `//:tidy_tcl` | `sh_binary` | Runs `tclfmt --in-place .` (auto-format) |

## Configuration

TCL linting and formatting are controlled by `tclint.toml` at the repository
root. See the [tclint documentation](https://tclint.readthedocs.io/) for
available options.

## POLA

`//:fix_lint` follows the
[Principle of Least Astonishment](https://en.wikipedia.org/wiki/Principle_of_least_astonishment):
it fixes what it can (e.g. `tclfmt --in-place`) and exits with an error if
any lint violations remain that require manual intervention (e.g. `line-length`).
A developer who runs `fix_lint` before pushing should never be surprised by
a CI lint failure.

## Adding new linters

The umbrella targets (`//:lint_test` and `//:fix_lint`) are designed for extension.
To add a new linter (e.g., C++ tidy):

1. Add the tool dependency to `bazel/requirements.in` (pip) or as a Bazel
   module dependency
2. Create a wrapper script in `bazel/` (e.g., `bazel/cpp_tidy.sh`)
3. Add `sh_test` / `sh_binary` targets in `BUILD.bazel`
4. Wire into umbrellas:
   - Add the new test to the `//:lint_test` `test_suite`
   - Add a call in `bazel/fix_lint.sh`

## Planned additions

The following linters and formatters are planned for `//:lint_test` and
`//:fix_lint`, replacing their ad-hoc CI equivalents:

- **C++ clang-tidy** — static analysis for C++ sources
- **C++ clang-format** — formatting check/fix for C++ and header files
- **Python ruff** — lint + format for Python scripts in `etc/`, `docs/`, tests
- **Buildifier** — lint + format for BUILD, .bzl, and MODULE.bazel files
- **ShellCheck** — lint for bash scripts in `test/`, `bazel/`, `etc/`
- **Duplicate message ID check** — replace Jenkins "Find Duplicated Message IDs" stage
- **Doc consistency checks** — replace Jenkins "Documentation Checks" stage
  (`man_tcl_check`, `readme_msgs_check`)

The goal is to phase out hand-coded GitHub Actions YAML and Jenkins scripts in
favor of Bazel-managed targets that are reproducible, version-pinned, and
testable locally with a single command.

## Relationship to CI

The GitHub Actions workflow (`.github/workflows/github-actions-lint-tcl.yml`)
runs the same `tclint` and `tclfmt` checks. Once Bazel lint targets are
validated in CI, the GitHub Actions workflow can be retired. The same applies
to the Jenkins documentation and duplicate ID check stages.

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

# Auto-fix formatting
bazelisk run //:fix_lint
```

## Available targets

| Target | Type | What it does |
|--------|------|-------------|
| `//:lint_test` | `test_suite` | Umbrella: runs all lint checks |
| `//:fix_lint` | `alias` | Umbrella: auto-fixes what can be fixed |
| `//:lint_tcl_test` | `sh_test` | Runs `tclint .` (lint rules) |
| `//:fmt_tcl_test` | `sh_test` | Runs `tclfmt --check .` (formatting) |
| `//:tidy_tcl` | `sh_binary` | Runs `tclfmt --in-place .` (auto-format) |

## Configuration

TCL linting and formatting are controlled by `tclint.toml` at the repository
root. See the [tclint documentation](https://tclint.readthedocs.io/) for
available options.

Current settings:
- **Excluded**: `src/sta/`
- **Ignored rules**: `unbraced-expr`
- **Style**: 2-space indent, 100-char line length, spaces in braces

## Adding new linters

The umbrella targets (`//:lint_test` and `//:fix_lint`) are designed for extension.
To add a new linter (e.g., C++ tidy):

1. Add the tool dependency to `bazel/requirements.in` (pip) or as a Bazel
   module dependency
2. Create a wrapper script in `bazel/` (e.g., `bazel/cpp_tidy.sh`)
3. Add `sh_test` / `sh_binary` targets in `BUILD.bazel`
4. Add the new test to the `//:lint_test` `test_suite`

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

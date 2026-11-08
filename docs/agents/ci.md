# CI Workflow

## Pull Requests

### Never Close/Reopen a PR to Retrigger CI
Never close a PR to retrigger CI; it's visible to everyone. Push an empty commit instead:
```bash
git commit --allow-empty -s -m "retrigger CI"
```

### `gh pr checks` Exit Code 8
Exit code 8 means some checks are still pending -- not an error.

### Clang-Tidy CI
Clang-Tidy CI fails on any clang-tidy finding in the lines the PR changes; reviewdog posts the findings as review comments. Fix them and push; reproduce locally as below.

## What CI Runs

**Jenkins** first runs `bazel-build-tests`; only if it succeeds do
`bazel-ci` and the flow steps run, in parallel. `bazel-docs`, `bazel-nogui`,
and the Docker builds run alongside. Each step leaves a log named after it
(see CI Artifacts). The build and test steps pass `--config=ci` (remote
cache, needs credentials), `--//:platform=gui` (the local default is
`cli`), and `--force_pic`. Drop `--config=ci` when reproducing locally;
keep the other two.

| Step | Reproduce locally |
|------|-------------------|
| `bazel-build-tests` | `bazelisk build --build_tests_only --keep_going --//:platform=gui --force_pic ... //test:<design>_<platform>-tcl_test...` (all 14 flow tests) |
| `bazel-ci` (all tests) | `bazelisk test --test_output=errors --//:platform=gui --force_pic ...` |
| `bazel-flow-<design>_<platform>` | `bazelisk test --test_output=errors --//:platform=gui --force_pic //test:<design>_<platform>-tcl_test` |
| `bazel-docs` | `bazelisk build //docs:man_pages` |
| `bazel-nogui` | `bazelisk build --//:platform=cli //:openroad` |

- Flow tests are tagged `manual`, so `...` skips them; name them
  explicitly. Each checks `test/<design>_<platform>.metrics_limits`.
- Jenkins also builds the Docker image on six Linux distros and
  smoke-tests the binary, which exercises `etc/DependencyInstaller.sh`.
- `[skip ci]` or `[ci skip]` in the latest commit message skips the
  Bazel build-tests, test, and flow steps and the Docker smoke tests;
  docs, nogui, and the Docker image builds still run.

**GitHub Actions:**
- C++ format on changed files (`bazelisk test //:fmt_cc_test` runs the
  same check on all files).
- Clang-Tidy on the PR diff (see above).
- Tcl lint, Bazel lint (buildifier), and Python format (black). Run
  `bazelisk test //:lint_test` to check these locally, plus C++ and YAML
  format.
- `MODULE.bazel.lock` is up to date, generated ODB and web-bundle files
  are in sync, and no `.ok` file contains `Differences found`.
- macOS Bazel build.

## Running clang-tidy Locally via Bazel
Run clang-tidy hermetically using the toolchain Bazel already uses to compile:
```bash
# Single module:
bazelisk build --config=lint //src/utl/...

# Full lint scope (excludes the upstream OpenSTA submodule):
bazelisk build --config=lint -- //src/... //third-party/... -//src/sta/...
```
- Uses `clang-tidy` from `@llvm//tools` (the same binary `etc/run-clang-tidy.sh` uses).
- Reports land at `$(bazelisk info bazel-bin)/<pkg>/<target>_rules_lint/<src>.AspectRulesLintClangTidy.out`.
- Generated files (SWIG, bison, flex) are auto-skipped. Targets tagged `no-lint` are skipped.
- Aspect + config defined in `tools/lint/` and `.bazelrc` (`--config=lint`).

## CI Artifacts

Every Jenkins build of a PR publishes its logs at
`https://jenkins.openroad.tools/job/OpenROAD-Public/view/change-requests/job/PR-<N>-merge/<build>/artifact/`
(`PR-<N>-head` for the unmerged head).

- Each CI step leaves `bazel-<step>-attempt<k>.log.gz` (e.g., `bazel-ci-`,
  `bazel-build-tests-`, `bazel-nogui-`, `bazel-docs-`); each flow design
  leaves `bazel-flow-<design>_<platform>-attempt<k>.log.gz`. A higher
  `<k>` is a retry.
- Fetch only the log you need:
  `curl -sL <artifact-url>/<file>.log.gz | gunzip | less`.
  `*zip*/archive.zip` holds everything but is rarely needed.
- `build.profile` is a Bazel JSON trace profile (open it in
  `chrome://tracing` or Perfetto) for slow builds.

## Private-to-Staging Sync

To sync a private repo PR to the staging (public) repo:
1. Push commits to the **private repo** (`The-OpenROAD-Project-private/OpenROAD`)
2. Add the label **"Ready To Sync Public"** on the **private repo PR**

Don't push to the staging repo directly; the sync bot owns it.

**Important**: The label is **consumed** after each sync (removed by the sync bot). After pushing new commits, you must **re-add** the label. If the label already exists, remove it first, then re-add to trigger the workflow.

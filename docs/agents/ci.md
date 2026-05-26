# CI Workflow

## Pull Requests

### Never Close/Reopen a PR to Retrigger CI
Never close a public PR to retrigger CI -- `gh pr reopen` requires **admin permissions**. To retrigger CI, use empty commits instead:
```bash
git commit --allow-empty -s -m "retrigger CI"
```

### `gh pr checks` Exit Code 8
Exit code 8 means some checks are still pending -- not an error.

### Clang-Tidy CI
Clang-Tidy CI can fail not only for code issues but also for **unresolved review comments** on the PR. Review threads cannot be resolved via API on public repos -- must be done on the GitHub web UI.

### Running clang-tidy locally via Bazel
Run clang-tidy hermetically using the toolchain Bazel already uses to compile:
```bash
# Single module:
bazel build --config=lint //src/utl/...

# Full lint scope (excludes submodules):
bazel build --config=lint -- //src/... //third-party/... -//src/sta/... -//third-party/abc/...
```
- Uses `clang-tidy` from `@llvm_toolchain` (the same binary `etc/run-clang-tidy.sh` uses).
- Reports land at `$(bazel info bazel-bin)/<pkg>/<target>_rules_lint/<src>.AspectRulesLintClangTidy.out`.
- Generated files (SWIG, bison, flex) are auto-skipped. Targets tagged `no-lint` are skipped.
- Aspect + config defined in `tools/lint/` and `//:.bazelrc` (`--config=lint`).

## CI Artifacts

- Use distinct filenames for public vs private artifacts (never overwrite one with the other)
- Files >4GB may need `7z x` or `python3 -m zipfile -e` instead of `unzip`
- When possible, download only specific `metadata.json` files instead of the full archive
- Do not commit `metadata-base-ok.json` -- we have decided not to manage this file.

## Private-to-Staging Sync

To sync a private repo PR to the staging (public) repo:
1. Push commits to the **private repo** (`The-OpenROAD-Project-private/OpenROAD`)
2. Add the label **"Ready To Sync Public"** on the **private repo PR**

**Important**: The label is **consumed** after each sync (removed by the sync bot). After pushing new commits, you must **re-add** the label. If the label already exists, remove it first, then re-add to trigger the workflow.

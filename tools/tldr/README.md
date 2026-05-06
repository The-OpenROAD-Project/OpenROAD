# tldr — summarize CI failures on a PR

Implements [The-OpenROAD-Project/OpenROAD#10335](https://github.com/The-OpenROAD-Project/OpenROAD/issues/10335).

Turns the wall of CI-failure text on a PR into a curated, scannable
summary. **Inert and AI-free**: nothing inside this package talks to an
LLM, runs on a schedule, or posts to GitHub unless a human explicitly
runs the binary with `--post`.

## Usage

```bash
# Zero args: derive the current branch's open PR via git+gh, fetch the
# latest CI status, print a terminal-friendly table to stdout.
bazelisk run //:tldr

# By PR number:
bazelisk run //:tldr -- --pr 12345

# Render the would-be PR-comment Markdown (sentinel-bracketed block):
bazelisk run //:tldr -- --pr 12345 --format markdown

# Cross-repo:
bazelisk run //:tldr -- --pr 4204 --repo The-OpenROAD-Project/OpenROAD-flow-scripts

# Auto-fix the deterministic subset of findings (clang-format, buildifier,
# black, lockfile regen). AI-free. Findings that need human judgment are
# listed under "Still needs you" and the command exits non-zero.
bazelisk run //:fix-tldr

# Run all unit tests:
bazelisk test //tools/tldr/...
```

## Output: bracketed Markdown for PR upsert

When invoked with `--format markdown`, the tool emits a block bounded by
paired sentinels:

```
<!-- tldr:begin -->
... summary ...
<!-- tldr:end -->
```

The block has two stacked sections, in this order:

1. **TL;DR for humans** — three to six plain-English bullets a contributor
   can read in five seconds.
2. **Instructions for an AI coding tool** — a structured agent prompt
   with project norms, per-finding directives, verification commands,
   and a "done when" line. A contributor delegating the fix pastes
   `fix <PR-URL>` into their AI coding tool, which consumes this section.

The publisher upserts the block into the PR body in place — only the
bracketed span is touched, the rest of the body is byte-exact.

## Future GitHub Actions integration (not in this repo yet)

The plan for issue #10335 includes posting summaries automatically when
CI fails. Maintainers will wire that up after the tool has been
beta-tested manually. The workflow YAML below is recorded here for that
future step. **It is not added to `.github/workflows/` in this PR.**

```yaml
# Drop this into .github/workflows/summarize-ci.yml when ready to enable.
name: Summarize CI failures
on:
  check_run:
    types: [completed]
  status: {}
permissions:
  pull-requests: write
  checks: read
  statuses: read
jobs:
  summarize:
    if: github.event.check_run.conclusion == 'failure' || github.event.state == 'failure'
    runs-on: ${{ vars.USE_SELF_HOSTED == 'true' && 'self-hosted' || 'ubuntu-latest' }}
    steps:
      - uses: actions/checkout@v4
      - if: vars.USE_SELF_HOSTED != 'true'
        run: |
          curl -fsSL -o /usr/local/bin/bazelisk \
            https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64
          chmod +x /usr/local/bin/bazelisk
      - env:
          GH_TOKEN: ${{ secrets.GITHUB_TOKEN }}
        run: |
          bazelisk run //:tldr -- \
            --repo "${{ github.repository }}" \
            --sha "${{ github.event.check_run.head_sha || github.event.sha }}" \
            --post
```

## Design

Pipeline:

```
CI status (gh API) ─► fetch logs ─► strip ANSI/timestamps ─► stage tracking ─►
  ─► [parser plugins] ─► dedupe ─► [render_table | render_markdown] ─► stdout/PR
```

Parser plugins live in `src/tldr/parsers/`. Each plugin:

- declares which check names / repos it applies to,
- scans a stream of stripped log lines under a `StageContext`,
- yields zero or more `Finding` records.

`Finding` carries enough metadata to render every output mode: a one-line
`headline`, a longer `detail`, a `dedupe_key`, and three optional fields
(`human_headline`, `ai_directive`, `verify_command`,
`auto_fix_command`) that drive the human TL;DR, the AI-directive
section, and the deterministic auto-fix recipe.

Stdlib only. No `pip install` is needed for any code path.

## Layout

```
tools/tldr/
├── BUILD.bazel
├── pyproject.toml
├── README.md (you are here)
├── src/tldr/
│   ├── __main__.py        # CLI entry point (also the py_binary main)
│   ├── cli.py
│   ├── fix_main.py        # //:fix-tldr entry point — deterministic auto-fix
│   ├── local.py           # zero-arg path: git rev-parse + gh pr view
│   ├── github_api.py
│   ├── fetch.py
│   ├── strip.py
│   ├── stages.py
│   ├── dedupe.py
│   ├── render_table.py
│   ├── render_markdown.py
│   ├── publish.py
│   └── parsers/...
├── config/repos.toml      # per-repo enabled parsers
└── tests/                 # stdlib unittest, exercised by `bazelisk test //tools/tldr/...`
```

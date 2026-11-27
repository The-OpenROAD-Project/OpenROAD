---
name: design-smells
description: >
  Audit OpenROAD C++ modules for design smells: Feature Envy, God Class,
  Data Class, Message Chains, Inappropriate Intimacy, Shotgun Surgery,
  Middle Man, Long Method, Long Parameter List, and Data Clumps. It
  measures every class, method, and function with a libclang AST pass,
  then reads
  and triages each candidate before reporting it with a concrete
  refactoring. Use when asked to find refactoring opportunities, design
  smells, code smells, misplaced methods, classes that do too much, poor
  cohesion or encapsulation, Law of Demeter violations, overlong
  functions or parameter lists, parameters that always travel together,
  or "where does this logic belong" across a module or the codebase,
  even if no smell is
  named. Not for reviewing a diff or PR (use review-pr, /code-review, or
  /simplify).
---

# Design smell audit

This skill finds code that sits in the wrong place or has the wrong
shape. That includes methods that envy another class's data, classes
that do too much or too little, coupling through navigation chains or
widely scattered callers, and functions that are too long or take
parameter groups that should be a type. It works in three stages:

1. `run.sh extract` parses the sources with libclang and writes a
   **facts** JSON with per-class and per-method metrics. This is slow.
2. `run.sh detect` turns the facts into ranked candidates for each smell.
   This is instant, and you can rerun it with different thresholds.
3. **You triage** the candidates by reading the code. The metrics find
   suspects. Your reading decides whether a refactoring is actually
   worth doing, and only those become findings.

Stage 3 is the point of the skill. A raw list of metric hits is mostly
serializers, large engine classes, and established idioms. Passing it to
the user unfiltered wastes their time.

Run every command from the OpenROAD repo root. Paths in `compile_flags.txt`
are relative to it.

## Prerequisites

- **Compile database:** run `etc/bazel-make-compilation-db.sh`. It takes
  about a minute when the Bazel cache is warm and writes `compile_flags.txt`
  (gitignored). Its include paths go through the `bazel-bin` symlink,
  which follows whichever configuration Bazel built last. After any other
  `bazelisk build/run/test` (even `//:tidy_py`), generated headers such as
  `StaConfig.hh` and `omp.h` stop resolving. Rerun the script whenever
  extraction warns about missing headers, and after `MODULE.bazel` or
  BUILD changes.
- **libclang:** any host LLVM 17 or newer. `run.sh` uses the newest
  `/usr/lib/llvm-*` (or Homebrew `llvm` on macOS), or `LIBCLANG=` and
  `CLANG_RESOURCE_DIR=` if you set them. On first use it installs the
  matching `clang` Python bindings into
  `~/.cache/openroad-design-smells/`.

## 1. Extract

```bash
out=<scratch dir>    # not the repo
bash .agents/skills/design-smells/run.sh extract openroad/rsz -o $out/rsz.json
```

Pass one or more module directories, or single files. Only classes,
methods, and free functions defined under the given paths are recorded,
including those in their headers. A single file widens the recorded
scope to its Bazel package (e.g. `openroad/rsz`), so its classes are
found. Only that file's method bodies are parsed, though, so
class-level metrics such as WMC are partial. Pass the module directory
for class-level smells. Shotgun Surgery counts callers, so it
only sees callers inside the extracted paths. Run it on facts for all of
`openroad/` when callers in other modules matter.

Rough times on 64 cores: about 10 s for a mid-size module, and 1.5
minutes for all of `openroad/` (a 38 MB facts file).
Scale the time by the core count on smaller machines, and run the whole
tree in the background.

Extraction skips vendored and generated code by default: third-party,
tests, OpenSTA, the Si2 LEF/DEF parsers, gpl's copy of the Ooura FFT,
and SWIG wrappers. It also
skips `gpl/src/gpu` (Kokkos) and `syn/src/elab` (slang), because
`compile_flags.txt` lacks their headers. Override this with `--exclude`.
Files that still fail to parse are listed in the facts and in detect's
header. Their results may be missing members whose types did not resolve.

## 2. Detect

```bash
bash .agents/skills/design-smells/run.sh detect $out/rsz.json            # all smells
bash .agents/skills/design-smells/run.sh detect $out/rsz.json \
    --smell feature-envy --smell data-class --top 40 --json $out/rsz-findings.json
```

Useful options:

- `--path REGEX` limits the report to matching files. This is handy when
  the facts cover several modules.
- `--clusters` lists each God Class's groups of fields and methods,
  which are the candidate classes to extract.
- `--include-data` and `--include-aggregates` bring back options-struct
  cases.

Each smell has its own threshold flags (see `--help`). The message-chain
report has two tables: long chains, and repeated sub-chains (short
navigation repeated at many sites, usually the more actionable of the
two). Long Method and Long Parameter List list hundreds of items on a
large codebase. Their defaults follow the repo's ~100-line function
guideline. Use them to support other findings, or report the top few,
rather than triaging every row.

`references/smells.md` defines every metric and threshold, gives the
standard refactoring for each smell, and lists OpenROAD's known false
positives. Read it before triaging.

The smells reinforce each other. A Data Class with Feature Envy methods
pointing at it is the strongest signal, because the fix is to move those
methods in. The Data Class table's `envied by` column shows that link.

## 3. Triage

Go through the top candidates for each requested smell, in report order.
For each one, read the method or class with enough context to judge it.
Usually that means the code itself, the envied class's header, and the
callers. Then decide:

- **Is it one of the known false positives?** These include serializers,
  descriptors, options plumbing, generated odb code, and algorithms that
  operate on a central graph by design. See the list in
  `references/smells.md`. If so, drop it.
- **For a God Class, is there a real split?** Run with `--clusters`. One
  giant group means no clean Extract Class. Drop the candidate unless a
  smaller group is a coherent concept of its own.
- **For Shotgun Surgery, is there a protocol to hide?** A stable,
  widely used data-model getter is fine. A finding is a call pattern
  that every caller has to repeat correctly, such as a begin/end pair or
  a fixed sequence of calls. That can become one method.
- **For a Data Clump, does a type already exist?** `lx, ly, ux, uy` is
  `odb::Rect`, and `x, y` is `odb::Point`. Proposing an existing type is
  stronger than inventing a struct.
- **Would the refactoring make the code clearer?** Name the specific move.
  For example: "move `getWireLengthGradientPinWA` to `GPin`; it only reads
  `GPin`/`GNet` state and `log_`". If you can't name one, drop the
  candidate.
- **Does the dependency direction allow it?** Moving tool logic into odb
  or utl, or between peer modules, is usually wrong. Peer-module
  dependencies go through `utl::ServiceRegistry` interfaces, and
  OpenSTA is never touched. See "Choosing where code should live" in the
  reference.
- **Is it a hot loop?** In gpl Nesterov, drt, or grt inner loops, a
  move that adds indirection needs a benchmark. Say so instead of calling
  the change free.

For a large audit (a whole module with many candidates), you can hand
groups of candidates to subagents. Give each one the facts entry, this
triage list, and the reference file. Verify their conclusions before you
report them.

## Report

Lead with the confirmed findings, strongest first. For each one, give:

- **Location and smell:** `file:line`, the method or class, and the
  metrics that flagged it.
- **Evidence:** in a sentence or two, what it reaches into and what it
  doesn't use of its own.
- **Refactoring:** the specific move, and where the code would go.
- **Caveats:** dependency direction, hot-path risk, test impact.

After the findings, list in one line each the candidates you looked at
and rejected, with the reason (e.g. "`UPFWriter::writeLevelShifter`:
serializer"). This shows the triage happened and keeps the user from
re-checking them. End with the parse-error count and any excluded paths
if they affect coverage.

If the user wants a shareable report, publish it as an artifact. For a
quick answer, the terminal is fine.

## Applying the refactorings

When the user asks you to make the changes, run clang-tidy on the result
before committing. Extracted functions, new classes and moved code
regularly pick up findings that CI's clang-tidy check flags on the PR
diff: missing direct includes, signed/unsigned loop bounds, and missing
trailing commas in designated initializers. `//:lint_test` does not run
clang-tidy.

```bash
bin=$(bazelisk info bazel-bin)
find "$bin/openroad/<module>" -name '*.AspectRulesLintClangTidy.out*' -delete
bazelisk build --config=lint -- //openroad/<module>/...
bash tools/lint/collect_clang_tidy.sh "$bin/openroad/<module>" $out/tidy.txt
```

Pass the module's directory under `bazel-bin`, not `bazel-bin` itself.
Stale outputs from other modules make the collector report tool
failures. Fix the findings on lines you changed, and the include-cleaner
findings your change caused, such as a header you made unused. Then
format with `bazelisk run //:tidy_cc` and rerun the module's tests.

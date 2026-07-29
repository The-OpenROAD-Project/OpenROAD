---
name: contract-review
description: >
  Adversarial contract-surface review of a diff or branch. Instead of
  validating test trajectories, it reasons over the contract surface of
  every touched abstraction: builds an input/mode matrix per API, sweeps
  sibling code paths, greps for broken invariants, checks every documented
  claim against enforcing code, and diffs initialization across entry
  points. Finds the bugs tests structurally cannot: silent-empty results,
  unreachable-in-fixture crashes, lifecycle gaps, stale sibling paths.
  Use before handing changes to an external reviewer, after landing a fix
  family, or when asked to "review like Codex", "find what tests miss",
  "audit the surface", "adversarial review", "contract review".
argument-hint: "[base-ref (default: merge-base with master)]"
---

# Contract-Surface Review

Review the working diff (or `$ARGUMENTS` base ref) the way a strong static
reviewer does: per touched abstraction, enumerate what the *contract*
promises for every input class — not what the tests happen to exercise.
Tests validate trajectories; this validates the surface.

## Step 0 — Scope

```bash
BASE=${ARGUMENTS:-$(git merge-base HEAD master)}
git diff --stat $BASE
git diff $BASE > /tmp/review.diff
```

Read the diff once, whole. Then IGNORE it as a boundary: the diff tells you
which surfaces changed; the review examines those surfaces in full, not
just the changed lines.

## Step 1 — Surface list

Enumerate every abstraction the diff touches or newly depends on:
- public methods / API entry points (including overrides of a base class),
- iterators and visitors,
- callbacks/observers the code attaches or handles,
- command/Tcl entry points,
- serialization/restore paths.

For overrides: list the WHOLE family from the header, not just the
functions edited — the un-edited siblings are where the bugs hide.

## Step 2 — Mode matrix

Derive the input/mode dimensions **from the code itself**, then classify
every (surface × mode) cell.

How to find the dimensions:
- every `if (modeX())` / feature flag / null-object state that changes
  behavior (e.g. hierarchy on/off, special top objects, null parents),
- every input class with a distinct representation (object kinds behind a
  common handle, tagged pointers, empty/unbound/spare variants),
- every lifecycle state (fresh-parse vs restored-from-disk vs
  incrementally edited),
- every environment multiplicity the model allows (multiple blocks, techs,
  libraries, corners).

Classify each cell as one of:
- **handled** — explicit branch, correct;
- **guarded** — rejected loudly (error/warn) before reaching the code;
- **unreachable** — prove WHY (a validator upstream, an invariant), don't
  assert it;
- **UNKNOWN** — this is a finding. No cell may remain unknown at the end.

Watch for the two failure classes separately:
- **crash-class**: unguarded dereference (nulls, default-constructed
  iterators compared, casts on the wrong object kind);
- **silent-class**: returns empty/false/0/"" for a mode with no branch —
  worse than a crash, because nothing tells the user.

## Step 3 — Sibling sweep

For every function the diff FIXES, name its family and check each member
for the same defect:
- literal lookup ↔ wildcard/pattern matching,
- find-one ↔ iterate-all,
- name() ↔ pathName() ↔ id(),
- read ↔ write ↔ restore,
- forward query ↔ reverse query (net-of-pin ↔ pins-of-net),
- the singular ↔ plural API (`driver()` ↔ `drivers()`).

A fix that lands on one sibling and not the others is a finding even if no
test fails.

## Step 4 — Invariant grep

Grep the touched files for assumptions the change's own model breaks.
Build the pattern list from the diff's design, e.g.:
- singleton accessors (`getTech()`, `getChip()`, global block/module
  handles) in code that now supports multiplicity,
- direct member derefs (`x_->`) where `x_` is now nullable,
- "the design is one X" loops in code that now has many X.

Every hit: guarded, provably-safe, or finding.

## Step 5 — Claim-vs-code

Take every behavioral claim in the PR description, commit messages, and
code comments ("X is legal", "Y is derived from Z", "spares are
filtered"). For each claim, point to the code that ENFORCES or CONSUMES
it, and check consistency. A claim with no enforcing code, or code that
contradicts the claim (a counter that excludes what the claim calls
common), is a finding.

## Step 6 — Lifecycle parity

List every entry point that should yield equivalent state (parse-from-
source vs load-from-binary; initial build vs rebuild; first call vs cached
call). Diff their initialization sequences side by side. Missing init in
one path = finding. Duplicated init across caller and callee (common after
merges) = finding.

## Step 7 — Event-surface audit

For every callback/observer the code attaches: list what fires it, and
what the handler assumes about its owner/argument. Handlers written for
one owner class attached to another (e.g. block-level callbacks attached
to sub-blocks) are findings even if currently unreachable — state the
reachability honestly.

## Step 8 — Report

Output findings as a table: `surface | mode/input | class
(crash/silent/lifecycle/sibling/claim/event) | line(s) | one-line failure
scenario`. Rank by likelihood a *supported user flow* hits them. State
explicitly which cells were verified as handled/guarded — the absence
list is as important as the findings list.

Only after the report, optionally run the test suites — as confirmation,
never as the review.

## Anti-patterns (the misses this skill exists to prevent)

- Working around an anomaly in your own testing ("wildcard didn't work, I
  used exact names") instead of filing it as a finding.
- Auditing only for crashes and missing the silent-empty class.
- Declaring a cell unreachable without naming the guard that makes it so.
- Reviewing only the changed lines of a function family and skipping the
  untouched siblings.
- Letting the fixture define coverage: if the fixture has no spare/empty/
  restored/multi-X case, the matrix still must answer for those modes.

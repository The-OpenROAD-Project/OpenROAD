# Generated-via enclosure regression

These synthetic fixtures exercise ordinary `pdngen` and inspect the actual
OpenDB cut and landing rectangles. They require no external design, netlist,
checkpoint, or input database. They reference the existing
`test/Nangate45/Nangate45.lef`; that file and its notices are unchanged.

The affected baseline is
`80c6be93c28244f9f72840852a667daef764a4ff`.
Build the baseline and the proposed revision with the same dependencies using
the repository's [build instructions](../../../../docs/user/Build.md).
Use complete OpenROAD executables with Tcl/OpenDB support. Bash and `cmp` are
needed for the commands below. The ordinary PDN suite also includes Python
tests, which require the executable's matching Python runtime.

## Cases and assertions

| Case | Original geometry in the affected dimension | Corrected behavior |
| --- | --- | --- |
| `edge` | 140 nm cut, 20 nm landing: enclosure is -60 nm on each side, below the zero minimum | Reject the malformed generated array |
| `positive` | 140 nm cut, 150 nm landing: 5 nm enclosure, below a synthetic 10 nm minimum | Reject that generated array; a separately legal fixed-via fallback is allowed |
| `valid` | Sufficient overlap with the original rule | Preserve legal geometry and both supplies |

At 2000 DBU/micron, the first two margins are `(40 - 280) / 2 = -120` DBU
and `(300 - 280) / 2 = 10` DBU; the positive case requires 20 DBU.
Only the positive fixture changes the generated rule's default, through
OpenDB. It does not change the fixed via's rules.

`audit.tcl` computes the four margins from actual rectangles without calling
PDN's fit predicate. These small fixtures have no cut-layer enclosure overrides;
the audit is intentionally a default-rule check, not a general DRC engine.
The tests also assert the macro pose and native VDD/VSS connectivity.

## Reproduce in a clean directory

From the proposed checkout, set `OPENROAD_BEFORE` and `OPENROAD_AFTER` to absolute
paths for the unpatched and corrected executables. Record their source revisions
and hashes separately; `-version` alone does not identify an uncommitted patch.

```bash
repo_root=$(git rev-parse --show-toplevel)
git rev-parse HEAD
sha256sum "$OPENROAD_BEFORE" "$OPENROAD_AFTER"
repro_dir=$(mktemp -d)
cp "$repo_root"/src/pdn/test/generated_via_default_*.tcl "$repro_dir/"
cp -R "$repo_root/src/pdn/test/generated_via_enclosure" "$repro_dir/"
cp "$repo_root/test/helpers.tcl" "$repro_dir/"
mkdir "$repro_dir/Nangate45"
cp "$repo_root/test/Nangate45/Nangate45.lef" "$repro_dir/Nangate45/"
cd "$repro_dir"
for arm in before after; do
  if [ "$arm" = before ]; then exe=$OPENROAD_BEFORE; else exe=$OPENROAD_AFTER; fi
  for case in edge positive valid; do
    if RESULTS_DIR="$repro_dir/$arm" "$exe" -no_splash -no_init -exit \
      "generated_via_default_${case}.tcl" >"${arm}_${case}.log" 2>&1; then
      status=0
    else
      status=$?
    fi
    echo "$arm $case exit=$status"
    cat "$arm/generated_via_default_$case/outcome.txt"
  done
done
for artifact in result.def via_geometry.tsv via_enclosures.tsv; do
  cmp "before/generated_via_default_valid/$artifact" \
      "after/generated_via_default_valid/$artifact"
done
```

Expected original exits are `1, 1, 0`; corrected exits are `0, 0, 0`.
Original invalid generated-instance counts are `1, 1, 0`; corrected counts
are all zero. Both supplies pass in all six runs. The three valid-control
comparisons are byte-identical. See `via_geometry.tsv` for the actual
rectangles and margins, and `via_enclosures.tsv` for instantiated arrays.
A successful corrected test can mean an illegal via was rejected; it does
not mean a replacement connection was constructed.

The three tests are also registered in both `src/pdn/test/BUILD` and
`CMakeLists.txt`. With a configured CMake build, run:

```bash
ctest --test-dir build -R '^pdn\.' -j 1 --output-on-failure
```

## Effective rules and signed rounding

`checkMinEnclosure()` requests applicable rules from
`getMinimumEnclosures(..., true)`. The generated-via implementation must retain
the VIARULE default separately for each landing when that landing has no
applicable cut-layer enclosure rule. When such rules exist, they remain the
validation alternatives; this patch does not impose an additional default.
The existing lookup filters cut class and above/below applicability and selects
the greatest applicable minimum-width tier. Existing orientation handling and
the shared-metal-width recheck remain in use.

This follows the default/override distinction in the
[LEF/DEF reference, Via Rule Generate](https://coriolis.lip6.fr/doc/lefdef/lefdefref/LEFSyntax.html).
The existing `stacked_via_merged_enclosure` regression exercises a
width-conditioned override and is retained unchanged.

An overlap deficit must also remain negative: integer division of -1 by 2
would truncate to zero. Floor division preserves the deficit, and negative
manufacturing-grid snapping rounds away from zero. The patch leaves the
existing legal landing-extension attempts available.

These checks do not certify full process DRC, replacement connectivity,
IR drop, electromigration, or full-chip timing and power.

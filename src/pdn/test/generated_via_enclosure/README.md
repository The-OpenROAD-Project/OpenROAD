# Generated-via enclosure tests

Enclosure correctness is covered in C++. One Tcl integration test checks
that the PDN rejection report and supply-connectivity checks work together.

## Native enclosure tests

`src/pdn/test/cpp/TestPDNVia.cpp` uses the in-memory PDN fixture to check
defaults on both landings, one-sided overrides, width applicability, valid
enclosure alternatives, signed rounding, and rejection of cuts with inadequate
enclosure. Run the existing target with:

```bash
bazelisk test --test_output=errors --runs_per_test=3 --cache_test_results=no \
  //src/pdn/test:pdn_via_unittest
```

On upstream `6f3f28cc85407d71ba64a550cca194d8a6a01c1c`, six new defect tests
fail in each of three executions. The valid-override control and all 204
pre-existing via tests pass. These tests do not depend on Tcl or external
technology files.

`TestPDNConnect.cpp` checks the caller as well. Containment, tapered-stack,
landing-extension and ripup controls use landings that meet the declared
enclosures. Their emitted rectangles are checked without calling the
generator's enclosure predicate. The original undersized locked, pinned,
outer-stack and nonpreferred-direction landings remain rejection cases.
On the same upstream revision, the three rejection tests fail while all
74 other connection tests pass, in each of three executions.

```bash
bazelisk test --test_output=errors --runs_per_test=3 --cache_test_results=no \
  //src/pdn/test:pdn_connect_unittest //src/pdn/test:pdn_via_unittest
```

## Integration coverage

`generated_via_default_edge.tcl` retains the cross-tool check not provided by
the via and connection unit tests. A fixed macro has a narrow M4/M5 overlap
that cannot hold the generated cut. `pdngen` must report that rejected VSS
connection while the other connections keep both supply nets connected.

The test uses the usual `helpers.tcl`, `make_result_file` and `diff_files`
pattern. Its three-line `.rptok` checks the failure type, net, layers and
location. Direct `check_power_grid` calls must pass for both VDD and VSS,
and the macro's pose must remain unchanged. No custom runner, geometry-table
writer, database dump or change of working directory is needed.

The former positive-minimum and valid-landing Tcl cases are covered by
`GeneratedDefaultEnclosureAppliesToBothLandings`,
`GeneratedBuildRejectsInsufficientDefaultEnclosure`, the caller rejection
tests and emitted-rectangle checks. Their additional copies of the grid
construction/connectivity sequence have been removed. Numeric enclosure
assertions remain in C++; the retained Tcl case is an integration check,
not a second enclosure-rule implementation.

The fixture references the existing `test/Nangate45/Nangate45.lef` unchanged.
It requires no external design, netlist, checkpoint or database.

```bash
bazelisk test --test_output=errors --runs_per_test=3 --cache_test_results=no \
  //src/pdn/test:generated_via_default_edge-tcl_test
```

Passing this fixture does not mean that every rejected connection can be
replaced. Here the supplies remain connected through other legal connections.

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

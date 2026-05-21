# Summary

Simple GCD (greatest common denominator) core. This is an extremely small
design mostly used to test the sanity of the flow.

Originally generated using [PyMTL](https://github.com/cornell-brg/pymtl).

This design has about 250 cells.

## Source

Re-used code derived from http://opencelerity.org/ project.

## Whittle test

The `gcd_whittle_test` is an integration test for `etc/whittle.py`, the
delta-debugging tool that minimises `.odb` files. It reuses the floorplan
`.odb` produced by the gcd flow and repeatedly runs global placement
(`global_placement -density 0.35 -skip_io`) while whittle removes
instances and nets until iteration 100 is no longer reached.

    bazelisk test //test/orfs/gcd:gcd_whittle_test

## Integrated-synthesis smoke test (`gcd_syn_test`)

`gcd_syn_test` is a throwaway smoke test that exercises OpenROAD's
**integrated synthesis tool** (`sv_elaborate` + `synthesize`, introduced
by [PR #10473](https://github.com/The-OpenROAD-Project/OpenROAD/pull/10473))
end-to-end on the gcd design:

1. A `genrule` (`:gcd_syn_netlist_gen`) invokes the local `openroad`
   binary with `synthesize_gcd.tcl`. The script reads the asap7 RVT/TT
   Liberty set, `sv_elaborate`s `gcd.v`, runs `synthesize`, and emits a
   mapped Verilog netlist (`gcd_syn_netlist.v`).
2. An `orfs_flow` target (`gcd_syn`) consumes that netlist via
   `SYNTH_NETLIST_FILES`, so floorplan → ... → final/test run the
   normal bazel-orfs flow against a syn-produced netlist instead of a
   Yosys-produced one.

Run it with:

    bazelisk test //test/orfs/gcd:gcd_syn_test

### Why does this live here instead of `orfs_synth`?

The proper home for this is `orfs_synth` in bazel-orfs, with something
like `SYNTH_HDL_FRONTEND="openroad"` letting users pick between the
Yosys-based frontend and OpenROAD's integrated `syn`. That requires:

* the integrated synthesis tool to land on OpenROAD master (PR #10473),
* matching bazel-orfs support to wire the new frontend into
  `orfs_synth`'s synthesis step, and
* the corresponding ORFS-side plumbing.

Until those land in their respective repositories, this test keeps the
end-to-end story self-contained in the OpenROAD repo to minimise
cross-repo churn. Once the proper integration ships, this target should
be replaced by an `orfs_synth(SYNTH_HDL_FRONTEND="openroad", ...)` call
and `synthesize_gcd.tcl` deleted. The integrated synthesis tool is brand
new; expect this test to fail until the syn implementation matures.

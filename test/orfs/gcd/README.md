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

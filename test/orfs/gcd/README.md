# Summary

Simple GCD (greatest common denominator) core. This is an extremely small
design mostly used to test the sanity of the flow.

Originally generated using [PyMTL](https://github.com/cornell-brg/pymtl).

This design has about 250 cells.

## Source

Re-used code derived from http://opencelerity.org/ project.

## LEC tests

LEC tests exist for each of the ORFS stages as well as a self-test for the original source.

In terms of expectations for LEC tests, this is experimental and contributions are highly appreciated. As of writing, even for such a small test case as gcd, the running time is considerable and eqy has false positives reported to the eqy project.

```bash
$ cd test/orfs/gcd
$ bazelisk query :* | grep eqy | grep _test
Loading: 0 packages loaded
//test/orfs/gcd:gcd_eqy_cts_test
//test/orfs/gcd:gcd_eqy_final_test
//test/orfs/gcd:gcd_eqy_floorplan_test
//test/orfs/gcd:gcd_eqy_grt_test
//test/orfs/gcd:gcd_eqy_place_test
//test/orfs/gcd:gcd_eqy_route_test
//test/orfs/gcd:gcd_eqy_source_test
//test/orfs/gcd:gcd_eqy_synth_test
//test/orfs/gcd:gcd_eqy_tests
```

To run all the tests:

    bazelisk test //test/orfs/gcd:gcd_eqy_tests --keep_going

## Whittle test

The `gcd_whittle_test` is an integration test for `etc/whittle.py`, the
delta-debugging tool that minimises `.odb` files. It reuses the floorplan
`.odb` produced by the gcd flow and repeatedly runs global placement
(`global_placement -density 0.35 -skip_io`) while whittle removes
instances and nets until iteration 100 is no longer reached.

    bazelisk test //test/orfs/gcd:gcd_whittle_test

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

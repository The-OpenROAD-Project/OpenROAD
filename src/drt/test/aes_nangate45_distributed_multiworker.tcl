# Workstream C: single-host MULTI-WORKER distributed detailed-route on
# aes_nangate45 (large enough to actually partition routing work across
# workers, unlike gcd). Routes the design with N local worker processes and
# asserts the routed DEF is byte-identical to the non-distributed baseline.
#
# Number of workers / cloud_size are taken from DIST_NUM_WORKERS /
# DIST_CLOUD_SIZE (defaults 2). The reusable orchestration is in
# distributed_multiworker_harness.tcl.
set DIST_DESIGN_NAME  aes_nangate45
set DIST_PREROUTE_DEF aes_nangate45_preroute.def
set DIST_GUIDE        aes_nangate45.route_guide
set DIST_BASELINE_TCL aes_nangate45.tcl
source "distributed_multiworker_harness.tcl"

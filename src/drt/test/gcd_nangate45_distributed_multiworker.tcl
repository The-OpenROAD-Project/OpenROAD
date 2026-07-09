# Workstream C: single-host MULTI-WORKER distributed detailed-route on
# gcd_nangate45, registered as a fast PASSFAIL regression. It routes the design
# with multiple local worker processes (default 2) + a balancer over localhost
# and asserts the routed DEF is byte-identical to the non-distributed baseline.
#
# This is the small/fast CI gate for the multi-worker path. For a design large
# enough to show real cross-worker routing speedup, use
# aes_nangate45_distributed_multiworker.tcl (heavier; run by hand or in a
# nightly). Number of workers / cloud_size are taken from DIST_NUM_WORKERS /
# DIST_CLOUD_SIZE (defaults 2). Orchestration is in
# distributed_multiworker_harness.tcl.
set DIST_DESIGN_NAME  gcd_nangate45
set DIST_PREROUTE_DEF gcd_nangate45_preroute.def
set DIST_GUIDE        gcd_nangate45.route_guide
set DIST_BASELINE_TCL gcd_nangate45.tcl
source "distributed_multiworker_harness.tcl"

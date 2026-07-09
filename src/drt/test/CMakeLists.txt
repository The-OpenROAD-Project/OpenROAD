or_integration_tests(
  "drt"
  TESTS
    down_access_term
    drc_test
    ispd18_sample
    ispd18_sample_incr
    ndr_vias1
    ndr_vias2
    ndr_vias3
    obstruction
    single_step
    ta_ap_aligned
    ta_pin_aligned
    top_level_term
    top_level_term2
    via_access_layer
  PASSFAIL_TESTS
    cpp_tests
    gcd_nangate45_distributed_local
)

set_tests_properties(
  drt.ndr_vias1.tcl
  drt.ndr_vias2.tcl
  PROPERTIES TIMEOUT 900
)

# The single-host distributed-route test spawns a leader + 2 worker processes
# + a balancer and routes gcd twice (baseline + distributed); give it headroom.
set_tests_properties(
  drt.gcd_nangate45_distributed_local.tcl
  PROPERTIES TIMEOUT 900
)

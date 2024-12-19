or_integration_tests(
  "drt"
  TESTS
    drc_test
    ispd18_sample
    ispd18_sample_incr
    ndr_vias1
    ndr_vias2
    obstruction
    single_step
    ta_ap_aligned
    ta_pin_aligned
    top_level_term
    top_level_term2
  PASSFAIL_TESTS
    gc_test
)

# Skipped
#drt_man_tcl_check
#drt_readme_msgs_check

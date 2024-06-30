record_tests {
  error1
  get_core_die_areas
  timing_api
  timing_api_2
  timing_api_3
  timing_api_4
  upf_test
  upf_aes
  two_designs
}

record_pass_fail_tests {
  commands_without_load
}

define_test_group "non_flow" {
  error1
  get_core_die_areas
  timing_api
  timing_api_2
  timing_api_3
  upf_test
  upf_aes
  two_designs
  commands_without_load
}

# Flow tests only check the last line in the log (pass/fail).
# Ordered by instance count.
record_flow_tests {
  gcd_nangate45
  gcd_sky130hd
  gcd_sky130hs

  ibex_sky130hd
  ibex_sky130hs

  aes_nangate45
  aes_sky130hd
  aes_sky130hs

  tinyRocket_nangate45

  jpeg_sky130hs
  jpeg_sky130hd
}

record_tests {
  error1
  get_core_die_areas
}

define_test_group "non_flow" {
  error1
  get_core_die_areas
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

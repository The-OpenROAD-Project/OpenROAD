record_tests {
  error1
  get_core_die_areas
}

define_test_group "non_flow" {
  error1
  get_core_die_areas
}

# Flow tests only check the last line in the log (pass/fail).
record_flow_tests {
  gcd_nangate45
  aes_nangate45
  tinyRocket_nangate45

  gcd_sky130hs
  aes_sky130hs
  jpeg_sky130hs

  gcd_sky130hd
  aes_sky130hd
  ibex_sky130hd
  jpeg_sky130hd
}
# rcx seg fault 05/16/2021
#  ibex_sky130hs

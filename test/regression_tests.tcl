record_tests {
  error1
}

# Flow tests only check the return code and do not compare output logs.
# These tests require TritonRoute in $PATH.
if {0} {
record_flow_tests {
  gcd_nangate45
  aes_nangate45
  tinyRocket_nangate45
}
}

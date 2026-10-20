# Use STA's actual float equality and the definition of sta::fuzzyLess.
proc hierarchy_timing_less { actual expected } {
  return [expr { $actual < $expected && ![sta::fuzzy_equal $actual $expected] }]
}

proc hierarchy_timing_equal { actual expected label } {
  if { ![sta::fuzzy_equal $actual $expected] } {
    error "$label is stale or does not match the selected placement: $actual vs $expected"
  }
}

proc report_metrics { when {include_erc true} } {
  puts "\n=========================================================================="
  puts "$when report_checks -path_delay min"
  puts "--------------------------------------------------------------------------"
  report_checks -path_delay min -fields {slew cap input nets fanout} -format full_clock_expanded

  puts "\n=========================================================================="
  puts "$when report_checks -path_delay max"
  puts "--------------------------------------------------------------------------"
  report_checks -path_delay max -fields {slew cap input nets fanout} -format full_clock_expanded

  puts "\n=========================================================================="
  puts "$when report_checks -unconstrained"
  puts "--------------------------------------------------------------------------"
  report_checks -unconstrained -fields {slew cap input nets fanout} -format full_clock_expanded

  if {$include_erc} {
    puts "\n=========================================================================="
    puts "$when report_check_types -max_slew -max_cap -max_fanout -violators"
    puts "--------------------------------------------------------------------------"
    report_check_types -max_slew -max_capacitance -max_fanout -violators

    puts "\n=========================================================================="
    puts "$when max_slew_violation_count"
    puts "--------------------------------------------------------------------------"
    puts "max slew violation count [sta::max_slew_violation_count]"

    puts "\n=========================================================================="
    puts "$when max_fanout_violation_count"
    puts "--------------------------------------------------------------------------"
    puts "max fanout violation count [sta::max_fanout_violation_count]"

    puts "\n=========================================================================="
    puts "$when max_cap_violation_count"
    puts "--------------------------------------------------------------------------"
    puts "max cap violation count [sta::max_capacitance_violation_count]"
  }

  puts "\n=========================================================================="
  puts "$when report_tns"
  puts "--------------------------------------------------------------------------"
  report_tns

  puts "\n=========================================================================="
  puts "$when report_wns"
  puts "--------------------------------------------------------------------------"
  report_wns

  puts "\n=========================================================================="
  puts "$when report_worst_slack"
  puts "--------------------------------------------------------------------------"
  report_worst_slack

  puts "\n=========================================================================="
  puts "$when report_clock_skew"
  puts "--------------------------------------------------------------------------"
  report_clock_skew

  puts "\n=========================================================================="
  puts "$when report_power"
  puts "--------------------------------------------------------------------------"
  report_power

  puts "\n=========================================================================="
  puts "$when report_design_area"
  puts "--------------------------------------------------------------------------"
  report_design_area

  puts ""
}

# Binding smoke test: check_antennas and the ant:: commands pass their
# arguments and results through Tcl. The C++ tests in cpp/ check the ratios.
source "helpers.tcl"

check_error "no design loaded" { check_antennas } ANT-0003

read_lef ant_check.lef
read_def ant_check.def

check "returns the violating net count" { check_antennas } 1
check "antenna_violation_count" { ant::antenna_violation_count } 1
check "-verbose" { check_antennas -verbose } 1
check "-net" { check_antennas -net net50 } 1
check_error "-net with an unknown net" { check_antennas -net xxx } ANT-0012
check_error "positional argument" { check_antennas net50 } STA-0564

tee -quiet -variable log { check_antennas -report_violating_nets }
check "-report_violating_nets is deprecated" \
  { string match {*ANT-0011*} $log } 1

check "check_net_violation" { ant::check_net_violation net50 } 1
check "check_net_violation on an unknown net" \
  { ant::check_net_violation xxx } 0

set report_file [make_result_file check_antennas_cmd.rpt]
check "-report_file" { check_antennas -report_file $report_file } 1
check "report file written" { expr { [file size $report_file] > 0 } } 1

exit_summary

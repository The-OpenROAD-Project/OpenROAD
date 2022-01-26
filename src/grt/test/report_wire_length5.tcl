# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "report_wire_length1.def"

read_guides "report_wire_length1.guide"

set report_file [make_result_file report_wire_length5.rpt]

report_wire_length -net {clk net60} -global_route -detailed_route -file $report_file

diff_file report_wire_length5.rptok $report_file
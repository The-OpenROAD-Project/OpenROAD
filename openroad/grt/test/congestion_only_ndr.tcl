# Verify that global_route -congestion_only estimates congestion on a design
# whose clock nets carry a non-default rule wider than one track: FastRoute's
# initial 2D routing reserves their per-layer NDR capacity, which
# -congestion_only must initialize as a full route does.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "clock_route.def"

create_ndr -name NDR \
  -spacing { li1 0.51 met1 0.42 met2 0.42 met3 0.9 met4 0.9 met5 4.8 } \
  -width { li1 0.34 met1 0.28 met2 0.28 met3 0.6 met4 0.6 met5 3.2 }
foreach net {clk clknet_0_clk clknet_2_0__leaf_clk clknet_2_1__leaf_clk
  clknet_2_2__leaf_clk clknet_2_3__leaf_clk} {
  assign_ndr -ndr NDR -net $net
}

set_routing_layers -signal met1-met5 -clock met3-met5
set_global_routing_layer_adjustment * 0.5

set rpt_file [make_result_file "congestion_only_ndr.rpt"]
global_route -congestion_only -allow_congestion \
  -congestion_report_file $rpt_file

check "-congestion_only with multi-track NDR nets writes a report file" {
  file exists $rpt_file
} 1

exit_summary

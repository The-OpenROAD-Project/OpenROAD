source "helpers.tcl"
read_lef "sky130hd/sky130hd.tlef"
read_lef "sky130hd/sky130hd_std_cell.lef"
read_def "gcd_sky130hd.def"
read_guides "gcd_sky130hd.guide"

set def_file [make_result_file ndr_vias3.def]

create_ndr -name NDR_3W_3S \
           -via { L1M1_PR_R M1M2_PR_R }

assign_ndr -ndr NDR_3W_3S -net clk
assign_ndr -ndr NDR_3W_3S -net clknet_0_clk
assign_ndr -ndr NDR_3W_3S -net clknet_2_0__leaf_clk
assign_ndr -ndr NDR_3W_3S -net clknet_2_1__leaf_clk
assign_ndr -ndr NDR_3W_3S -net clknet_2_2__leaf_clk
assign_ndr -ndr NDR_3W_3S -net clknet_2_3__leaf_clk

detailed_route -bottom_routing_layer met1 -top_routing_layer met5 -verbose 0

write_def $def_file
diff_files ndr_vias3.defok $def_file

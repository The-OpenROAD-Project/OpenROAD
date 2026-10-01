# Per-net disable of DRT auto-taper (issue #9995).
#
# Clock nets are given a wide NDR.  By default DRT auto-tapers NDR nets
# down to minimum width near pin connections.  Here we disable auto-taper
# on a subset of the NDR nets via set_routing_auto_taper so those
# nets keep their full NDR width all the way to the pin, while the other
# NDR nets continue to taper as before.  The DEF golden captures the
# resulting routing.
source "helpers.tcl"
read_lef "sky130hd/sky130hd.tlef"
read_lef "sky130hd/sky130hd_std_cell.lef"
read_def "gcd_sky130hd.def"
read_guides "gcd_sky130hd.guide"

set def_file [make_result_file ndr_no_auto_taper.def]

create_ndr -name NDR_3W_3S \
  -spacing { li1 0.51 met1 0.42 met2 0.42 met3 0.9 met4 0.9 met5 4.8 } \
  -width { li1 0.51 met1 0.42 met2 0.42 met3 0.9 met4 0.9 met5 4.8 } \
  -via { L1M1_PR_R M1M2_PR_R }

assign_ndr -ndr NDR_3W_3S -net clk
assign_ndr -ndr NDR_3W_3S -net clknet_0_clk
assign_ndr -ndr NDR_3W_3S -net clknet_2_0__leaf_clk
assign_ndr -ndr NDR_3W_3S -net clknet_2_1__leaf_clk
assign_ndr -ndr NDR_3W_3S -net clknet_2_2__leaf_clk
assign_ndr -ndr NDR_3W_3S -net clknet_2_3__leaf_clk

# Keep full NDR width to the pins on these nets (no auto-taper); the
# remaining NDR nets still taper by default.
set_routing_auto_taper -net clknet_2_0__leaf_clk -disable
set_routing_auto_taper -net clknet_2_1__leaf_clk -disable

set_routing_layers -signal met1-met5
detailed_route -verbose 0

write_def $def_file
diff_files ndr_no_auto_taper.defok $def_file

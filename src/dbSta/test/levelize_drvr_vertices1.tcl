# Test for Levelize::levelizedDrvrVertices() API.
# Verifies:
#   1. Returns driver vertices in non-decreasing level order
#   2. All returned vertices are drivers
source helpers.tcl
read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_verilog gcd_asap7.v
link_design -hier gcd
create_clock -name clk -period 500 {clk}

# Report first 100 driver vertices in level order and total count
sta::report_levelized_drvr_vertices 100

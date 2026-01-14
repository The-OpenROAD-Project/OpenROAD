# Test read_vcd
source "helpers.tcl"
read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef

read_verilog MockArray.v
read_verilog Element.v

link_design -hier MockArray

# Find a hierarchical module instance. It should not return NULL
set hier_mod_inst [sta::find_instance ces_0_0]
if { [get_name $hier_mod_inst] eq "NULL" } {
  puts "Error: Failed to find hierarchical module instance ces_0_0"
  exit 1
}

read_sdc MockArray.sdc

read_spef MockArray.spef
read_spef -path ces_0_0 Element.spef

read_vcd -scope TOP/MockArray MockArray.vcd

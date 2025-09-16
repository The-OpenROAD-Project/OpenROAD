source "helpers.tcl"
source flow_helpers.tcl

read_liberty "Nangate45/Nangate45_typ.lib"
read_lef Nangate45/Nangate45.lef

read_verilog gcd.v
link_design gcd

read_sdc gcd_nangate45.sdc

set spec_file [make_result_file write_artnet.spec]
write_artnet_spec -out_file $spec_file

diff_files write_artnet.specok $spec_file

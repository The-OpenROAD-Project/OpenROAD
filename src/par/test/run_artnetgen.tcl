source "helpers.tcl"

set design gcd
set spec_dir .
set verilog_dir .

if { ![file exists $spec_dir] } {
  file mkdir $spec_dir
}

read_liberty "Nangate45/Nangate45_typ.lib"
read_lef Nangate45/Nangate45.lef

read_verilog ${design}.v
link_design ${design}

read_sdc ${design}.sdc

write_artnet_spec -out_file ${spec_dir}/${design}.spec

exit

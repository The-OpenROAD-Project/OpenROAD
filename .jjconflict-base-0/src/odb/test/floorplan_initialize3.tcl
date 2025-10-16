# test SDC preservation
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog floorplan_initialize3.v
link_design top

set_max_fanout 5 [get_ports in1]

read_def -floorplan_initialize floorplan_initialize3.def

set sdc_file [make_result_file floorplan_initialize3.sdc]
write_sdc -no_timestamp $sdc_file
diff_files floorplan_initialize3.sdcok $sdc_file

# test for repairing vias
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x_noviarules.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_vias/floorplan_repair.def

repair_pdn_vias -all

set def_file [make_result_file repair_vias.def]
write_def $def_file
diff_files repair_vias.defok $def_file

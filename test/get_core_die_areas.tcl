# check accessors for die and core area in microns
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def get_core_die_areas.def

puts "[get_die_area]"
puts "[get_core_area]"

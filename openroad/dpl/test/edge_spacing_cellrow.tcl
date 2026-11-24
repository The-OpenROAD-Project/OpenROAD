# CELLROW edges with a half-height row in the floorplan
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_lef edge_spacing.lef
read_lef edge_spacing_cellrow.lef
read_def edge_spacing_cellrow.def
catch { check_placement -verbose } error
puts $error

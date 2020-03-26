source helpers.tcl
psn::set_log_pattern "\[%^%l%$\] %v"
puts [psn::has_transform constant_propagation]
read_liberty ../test/data/libraries/Nangate45/NangateOpenCellLibrary_typical.lib
read_lef ../test/data/libraries/Nangate45/NangateOpenCellLibrary.mod.lef
read_def constprop2.def

puts "No. Instances: [llength [get_cells *]]"
optimize_logic
puts "No. Instances: [llength [get_cells *]]"
set def_file [make_result_file constprop2.def]
write_def $def_file
diff_file $def_file constprop2.defok
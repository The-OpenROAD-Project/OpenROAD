set exp_folder ./exp
set design des_perf_a_md1 

set TIME_start [clock clicks -milliseconds]

opendp_external odp
odp import_lef iccad17-bench/${design}/tech.lef
odp import_lef iccad17-bench/${design}/cells_modified.lef
odp import_def iccad17-bench/${design}/placed.def

odp import_constraint ../iccad17-bench/${design}/placement.constraints
odp legalize_place

set TIME_taken [expr [clock clicks -milliseconds] - $TIME_start]

if {![file exists $exp_folder/]} {
  exec mkdir $exp_folder
}

set fp [open ./exp/${design}.rpt w]
set legality [odp check_legality]

puts $fp "Legality          : $legality"
puts $fp "Runtime           : $TIME_taken"
puts $fp "Sum displacement: : [odp get_sum_displacement]"
puts $fp "Avg displacement: : [odp get_average_displacement]"
puts $fp "Max displacement: : [odp get_max_displacement]"
puts $fp "Original HPWL     : [odp get_original_hpwl]"
puts $fp "Legalized HPWL    : [odp get_legalized_hpwl]"

close $fp



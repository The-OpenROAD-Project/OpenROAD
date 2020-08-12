
set design black_parrot 
set techDir ../nangate45-bench/tech 
set designDir ../nangate45-bench/design/${design}
set exp_folder exp

set TIME_start [clock clicks -milliseconds]

mplace_external mp

mp import_lef $techDir/NangateOpenCellLibrary.lef
set lefList [glob -directory ${designDir} *.lef]
foreach lef $lefList {
  mp import_lef $lef
}

mp import_def ${designDir}/${design}_tdms.def
mp import_verilog ${designDir}/${design}.v
mp import_sdc ${designDir}/${design}.sdc

mp import_lib $techDir/NangateOpenCellLibrary_typical.lib
set libList [glob -directory ${designDir} *.lib]
foreach lib $libList {
  mp import_lib $lib
}


mp import_global_config ${designDir}/IP_global.cfg

mp place_macros

mp set_plot_enable true

if {![file exists ${exp_folder}/]} {
  exec mkdir ${exp_folder}
}

mp export_all_def ./exp/${design}

set TIME_taken [expr [clock clicks -milliseconds] - $TIME_start]

set fp [open ./exp/${design}.rpt w]

puts $fp "Runtime     : $TIME_taken"
puts $fp "PossibleSol : [mp get_solution_count]"

exit

utl::set_metrics_stage "floorplan__{}"
source $::env(SCRIPTS_DIR)/load.tcl
erase_non_stage_variables floorplan
load_design 1_synth.v 1_synth.sdc

proc report_unused_masters {} {
  set db [ord::get_db]
  set libs [$db getLibs]
  set masters ""
  foreach lib $libs {
    foreach master [$lib getMasters] {
      # filter out non-block masters, or you can remove this conditional to detect any unused master
      if {[$master getType] == "BLOCK"} {
        lappend masters $master
      }
    }
  }

  set block [ord::get_db_block]
  set insts [$block getInsts]

  foreach inst $insts {
    set inst_master [$inst getMaster]
    set masters [lsearch -all -not -inline $masters $inst_master]
  }

  foreach master $masters {
    puts "Master [$master getName] is loaded but not used in the design"
  }
}

report_unused_masters

#Run check_setup
puts "\n=========================================================================="
puts "Floorplan check_setup"
puts "--------------------------------------------------------------------------"
check_setup

set num_instances [llength [get_cells -hier *]]
puts "number instances in verilog is $num_instances"

set additional_args ""
append_env_var additional_args ADDITIONAL_SITES -additional_sites 1

set use_floorplan_def [env_var_exists_and_non_empty FLOORPLAN_DEF]
set use_footprint [env_var_exists_and_non_empty FOOTPRINT]
set use_die_and_core_area [expr {[env_var_exists_and_non_empty DIE_AREA] && [env_var_exists_and_non_empty CORE_AREA]}]
set use_core_utilization [env_var_exists_and_non_empty CORE_UTILIZATION]

set methods_defined [expr {$use_floorplan_def + $use_footprint + $use_die_and_core_area + $use_core_utilization}]
if {$methods_defined > 1} {
    puts "Error: Floorplan initialization methods are mutually exclusive, pick one."
    exit 1
}

if {$use_floorplan_def} {
    # Initialize floorplan by reading in floorplan DEF
    puts "Read in Floorplan DEF to initialize floorplan:  $env(FLOORPLAN_DEF)"
    read_def -floorplan_initialize $env(FLOORPLAN_DEF)
} elseif {$use_footprint} {
    # Initialize floorplan using ICeWall FOOTPRINT
    ICeWall load_footprint $env(FOOTPRINT)

    initialize_floorplan \
        -die_area  [ICeWall get_die_area] \
        -core_area [ICeWall get_core_area] \
        -site      $::env(PLACE_SITE)

    ICeWall init_footprint $env(SIG_MAP_FILE)
} elseif {$use_die_and_core_area} {
    puts "x$::env(CORE_AREA)y [llength $::env(CORE_AREA)]"
    initialize_floorplan -die_area $::env(DIE_AREA) \
                         -core_area $::env(CORE_AREA) \
                         -site $::env(PLACE_SITE) \
                         {*}$additional_args
} elseif {$use_core_utilization} {
    set aspect_ratio 1.0
    if {[env_var_exists_and_non_empty "CORE_ASPECT_RATIO"]} {
        set aspect_ratio $::env(CORE_ASPECT_RATIO)
    }
    set core_margin 1.0
    if {[env_var_exists_and_non_empty "CORE_MARGIN"]} {
        set core_margin $::env(CORE_MARGIN)
    }
    initialize_floorplan -utilization $::env(CORE_UTILIZATION) \
                         -aspect_ratio $aspect_ratio \
                         -core_space $core_margin \
                         -site $::env(PLACE_SITE) \
                         {*}$additional_args
} else {
    puts "Error: No floorplan initialization method specified"
    exit 1
}

if { [env_var_exists_and_non_empty MAKE_TRACKS] } {
  source $::env(MAKE_TRACKS)
} elseif {[file exists $::env(PLATFORM_DIR)/make_tracks.tcl]} {
  source $::env(PLATFORM_DIR)/make_tracks.tcl
} else {
  make_tracks
}

if {[env_var_exists_and_non_empty FOOTPRINT_TCL]} {
  source $::env(FOOTPRINT_TCL)
}

if { [env_var_equals REMOVE_ABC_BUFFERS 1] } {
  # remove buffers inserted by yosys/abc
  remove_buffers
} else {
  repair_timing_helper 0
}

##### Restructure for timing #########
if { [env_var_equals RESYNTH_TIMING_RECOVER 1] } {
  repair_design_helper
  repair_timing_helper
  # pre restructure area/timing report (ideal clocks)
  puts "Post synth-opt area"
  report_design_area
  report_worst_slack -min -digits 3
  puts "Post synth-opt wns"
  report_worst_slack -max -digits 3
  puts "Post synth-opt tns"
  report_tns -digits 3

  write_verilog $::env(RESULTS_DIR)/2_pre_abc_timing.v

  restructure -target timing -liberty_file $::env(DONT_USE_SC_LIB) \
              -work_dir $::env(RESULTS_DIR)

  write_verilog $::env(RESULTS_DIR)/2_post_abc_timing.v

  # post restructure area/timing report (ideal clocks)
  remove_buffers
  repair_design_helper
  repair_timing_helper

  puts "Post restructure-opt wns"
  report_worst_slack -max -digits 3
  puts "Post restructure-opt tns"
  report_tns -digits 3

  # remove buffers inserted by optimization
  remove_buffers
}


puts "Default units for flow"
report_units
report_metrics 2 "floorplan final" false false

if { [env_var_equals RESYNTH_AREA_RECOVER 1] } {

  utl::push_metrics_stage "floorplan__{}__pre_restruct"
  set num_instances [llength [get_cells -hier *]]
  puts "number instances before restructure is $num_instances"
  puts "Design Area before restructure"
  report_design_area
  report_design_area_metrics
  utl::pop_metrics_stage

  write_verilog $::env(RESULTS_DIR)/2_pre_abc.v

  set tielo_cell_name [lindex $env(TIELO_CELL_AND_PORT) 0]
  set tielo_lib_name [get_name [get_property [lindex [get_lib_cell $tielo_cell_name] 0] library]]
  set tielo_port $tielo_lib_name/$tielo_cell_name/[lindex $env(TIELO_CELL_AND_PORT) 1]

  set tiehi_cell_name [lindex $env(TIEHI_CELL_AND_PORT) 0]
  set tiehi_lib_name [get_name [get_property [lindex [get_lib_cell $tiehi_cell_name] 0] library]]
  set tiehi_port $tiehi_lib_name/$tiehi_cell_name/[lindex $env(TIEHI_CELL_AND_PORT) 1]

  restructure -liberty_file $::env(DONT_USE_SC_LIB) -target "area" \
        -tiehi_port $tiehi_port \
        -tielo_port $tielo_port \
        -work_dir $::env(RESULTS_DIR)

  # remove buffers inserted by abc
  remove_buffers

  write_verilog $::env(RESULTS_DIR)/2_post_abc.v
  utl::push_metrics_stage "floorplan__{}__post_restruct"
  set num_instances [llength [get_cells -hier *]]
  puts "number instances after restructure is $num_instances"
  puts "Design Area after restructure"
  report_design_area
  report_design_area_metrics
  utl::pop_metrics_stage
}

if { [env_var_exists_and_non_empty POST_FLOORPLAN_TCL] } {
  source $::env(POST_FLOORPLAN_TCL)
}


if {[env_var_exists_and_non_empty IO_CONSTRAINTS]} {
  source $::env(IO_CONSTRAINTS)
}

write_db $::env(RESULTS_DIR)/2_1_floorplan.odb
write_sdc -no_timestamp $::env(RESULTS_DIR)/2_1_floorplan.sdc

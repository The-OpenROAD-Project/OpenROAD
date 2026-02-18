# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

sta::define_cmd_args "configure_cts_characterization" {[-max_cap cap] \
                                                       [-max_slew slew] \
                                                       [-slew_steps slew_steps] \
                                                       [-cap_steps cap_steps] \
                                                      }

proc configure_cts_characterization { args } {
  sta::parse_key_args "configure_cts_characterization" args \
    keys {-max_cap -max_slew -slew_steps -cap_steps} flags {}

  sta::check_argc_eq0 "configure_cts_characterization" $args

  if { [info exists keys(-max_cap)] } {
    set max_cap_value $keys(-max_cap)
    cts::set_max_char_cap $max_cap_value
  }

  if { [info exists keys(-max_slew)] } {
    set max_slew_value $keys(-max_slew)
    cts::set_max_char_slew $max_slew_value
  }

  if { [info exists keys(-slew_steps)] } {
    set steps $keys(-slew_steps)
    sta::check_cardinal "-slew_steps" $steps
    cts::set_slew_steps $slew
  }

  if { [info exists keys(-cap_steps)] } {
    set steps $keys(-cap_steps)
    sta::check_cardinal "-cap_steps" $steps
    cts::set_cap_steps $cap
  }
}

sta::define_cmd_args "set_cts_config" {[-apply_ndr strategy] \
                                       [-branching_point_buffers_distance] \
				       [-buf_list] \
                                       [-clock_buffer_footprint footprint] \
                                       [-clock_buffer_string string] \
                                       [-clustering_exponent] \
                                       [-clustering_unbalance_ratio] \
                                       [-delay_buffer_derate] \
                                       [-distance_between_buffers] \
                                       [-library] \
                                       [-macro_clustering_max_diameter] \
                                       [-macro_clustering_size] \
                                       [-num_static_layers] \
				       [-root_buf] \
                                       [-sink_buffer_max_cap_derate] \
                                       [-sink_clustering_levels levels] \
                                       [-sink_clustering_max_diameter] \
                                       [-sink_clustering_size] \
				       [-skip_nets] \
                                       [-tree_buf buf] \
	                               [-wire_unit unit]
}
proc set_cts_config { args } {
  sta::parse_key_args "set_cts_config" args \
    keys {-apply_ndr -branching_point_buffers_distance -buf_list \
           -clock_buffer_footprint -clock_buffer_string \
           -clustering_exponent \
           -clustering_unbalance_ratio -delay_buffer_derate -distance_between_buffers \
           -library -macro_clustering_max_diameter -macro_clustering_size \
           -num_static_layers -sink_buffer_max_cap_derate -sink_clustering_levels -root_buf \
           -sink_clustering_max_diameter -sink_clustering_size -skip_nets -tree_buf -wire_unit} \
    flags {}

  sta::check_argc_eq0 "set_cts_config" $args

  if { [info exists keys(-apply_ndr)] } {
    set strategy $keys(-apply_ndr)
    cts::set_apply_ndr $strategy
  }
  if { [info exists keys(-clock_buffer_string)] && [info exists keys(-clock_buffer_footprint)] } {
    utl::error CTS 135 "-clock_buffer_string and -clock_buffer_footprint are mutually exclusive."
  } elseif { [info exists keys(-clock_buffer_string)] } {
    if { ![rsz::has_clock_buffer_footprint_cmd] } {
      set clk_str $keys(-clock_buffer_string)
      rsz::set_clock_buffer_string_cmd $clk_str
    } else {
      utl::error CTS 133 "-clock_buffer_string cannot be set because\
        -clock_buffer_footprint is already defined."
    }
  } elseif { [info exists keys(-clock_buffer_footprint)] } {
    if { ![rsz::has_clock_buffer_string_cmd] } {
      set footprint $keys(-clock_buffer_footprint)
      rsz::set_clock_buffer_footprint_cmd $footprint
    } else {
      utl::error CTS 134 "-clock_buffer_footprint cannot be set because\
        -clock_buffer_string is already defined."
    }
  }
  if { [info exists keys(-branching_point_buffers_distance)] } {
    set distance $keys(-branching_point_buffers_distance)
    cts::set_branching_point_buffers_distance [ord::microns_to_dbu $distance]
  }
  if { [info exists keys(-buf_list)] } {
    set buf_list $keys(-buf_list)
    cts::set_buffer_list $buf_list
  }
  if { [info exists keys(-clustering_exponent)] } {
    set exponent $keys(-clustering_exponent)
    cts::set_clustering_exponent $exponent
  }
  if { [info exists keys(-clustering_unbalance_ratio)] } {
    set unbalance $keys(-clustering_unbalance_ratio)
    cts::set_clustering_unbalance_ratio $unbalance
  }
  if { [info exists keys(-delay_buffer_derate)] } {
    set buffer_derate $keys(-delay_buffer_derate)
    if { $buffer_derate < 0.0 } {
      utl::error CTS 129 "delay_buffer_derate needs to be greater than or equal to 0."
    }
    cts::set_delay_buffer_derate $buffer_derate
  }
  if { [info exists keys(-distance_between_buffers)] } {
    set distance $keys(-distance_between_buffers)
    cts::set_distance_between_buffers [ord::microns_to_dbu $distance]
  }
  if { [info exists keys(-library)] } {
    set cts_library $keys(-library)
    cts::set_cts_library $cts_library
  }
  if { [info exists keys(-macro_clustering_max_diameter)] } {
    set distance $keys(-macro_clustering_max_diameter)
    cts::set_macro_clustering_diameter $distance
  }
  if { [info exists keys(-macro_clustering_size)] } {
    set size $keys(-macro_clustering_size)
    cts::set_macro_clustering_size $size
  }
  if { [info exists keys(-num_static_layers)] } {
    set num $keys(-num_static_layers)
    cts::set_num_static_layers $num
  }
  if { [info exists keys(-root_buf)] } {
    set root_buf $keys(-root_buf)
    cts::set_root_buffer $root_buf
  }
  if { [info exists keys(-sink_buffer_max_cap_derate)] } {
    set derate $keys(-sink_buffer_max_cap_derate)
    if { $derate > 1.0 || $derate < 0.0 } {
      utl::error CTS 130 "sink_buffer_max_cap_derate needs to be between 0 and 1.0."
    }
    cts::set_sink_buffer_max_cap_derate $derate
  }
  if { [info exists keys(-sink_clustering_levels)] } {
    set levels $keys(-sink_clustering_levels)
    cts::set_sink_clustering_levels $levels
  }
  if { [info exists keys(-sink_clustering_max_diameter)] } {
    set distance $keys(-sink_clustering_max_diameter)
    cts::set_clustering_diameter $distance
  }
  if { [info exists keys(-sink_clustering_size)] } {
    set size $keys(-sink_clustering_size)
    cts::set_sink_clustering_size $size
  }
  if { [info exists keys(-skip_nets)] } {
    foreach net [get_nets $keys(-skip_nets)] {
      set db_net [sta::sta_to_db_net $net]
      cts::set_skip_clock_nets $db_net
    }
  }
  if { [info exists keys(-tree_buf)] } {
    set buf $keys(-tree_buf)
    cts::set_tree_buf $buf
  }
  if { [info exists keys(-wire_unit)] } {
    set wire_unit $keys(-wire_unit)
    cts::set_wire_segment_distance_unit $wire_unit
  }
}

sta::define_cmd_args "report_cts_config" {}

proc report_cts_config { args } {
  sta::parse_key_args "report_cts_config" args keys {} flags {}

  # Check if a design exists
  set db [ord::get_db]
  set chip [$db getChip]

  if { "$chip" eq "NULL" } {
    utl::error CTS 131 "No Chip exists"
  }

  # set the db unit
  cts::set_db_unit 0
  set ndr_strategy [cts::get_ndr_strategy]
  set buffer_list [cts::get_buffer_list]
  if { $buffer_list eq "" } {
    set buffer_list "undefined"
  }
  set vertex_buffer_distance [cts::get_branching_buffers_distance]
  set clustering_power [cts::get_clustering_exponent]
  set clustering_capacity [cts::get_clustering_unbalance_ratio]
  set delay_buffer_derate [cts::get_delay_buffer_derate]
  set buffer_distance [cts::get_distance_between_buffers]
  set cts_library [cts::get_library]
  if { $cts_library eq "" } {
    set cts_library "undefined"
  }
  set macro_max_diameter [cts::get_macro_clustering_max_diameter]
  set macro_sink_cluster_size [cts::get_macro_clustering_size]
  set num_static_layers [cts::get_num_static_layers]
  set root_buffer [cts::get_root_buffer]
  if { $root_buffer eq "" } {
    set root_buffer "undefined"
  }
  set sink_buffer_max_cap_derate [cts::get_sink_buffer_max_cap_derate]
  set sink_clustering_levels [cts::get_sink_clustering_levels]
  set sink_max_diameter [cts::get_sink_clustering_max_diameter]
  set sink_cluster_size [cts::get_sink_clustering_size]
  set skip_nets_list [cts::get_skip_nets]
  if { $skip_nets_list eq "" } {
    set skip_nets_list "undefined"
  }
  set tree_buffer [cts::get_tree_buf]
  if { $tree_buffer eq "" } {
    set tree_buffer "undefined"
  }
  set wire_segment_unit [cts::get_wire_unit]
  # reset the db units
  cts::set_db_unit 1
  set clock_buffer_string "unset"
  if { [rsz::has_clock_buffer_string_cmd] } {
    set clock_buffer_string [rsz::get_clock_buffer_string_cmd]
  }
  set clock_buffer_footprint "unset"
  if { [rsz::has_clock_buffer_footprint_cmd] } {
    set clock_buffer_footprint [rsz::get_clock_buffer_footprint_cmd]
  }
  puts "*****************************************************"
  puts "CTS config:"
  puts "-apply_ndr:                          $ndr_strategy"
  puts "-buf_list:                           $buffer_list"
  puts "-clock_buffer_footprint:             $clock_buffer_footprint"
  puts "-clock_buffer_string:                $clock_buffer_string"
  puts "-branching_point_buffers_distance:   $vertex_buffer_distance"
  puts "-clustering_exponent:                $clustering_power"
  puts "-clustering_unbalance_ratio:         $clustering_capacity"
  puts "-delay_buffer_derate:                $delay_buffer_derate"
  puts "-distance_between_buffers:           $buffer_distance"
  puts "-library:                            $cts_library"
  puts "-macro_clustering_max_diameter:      $macro_max_diameter"
  puts "-macro_clustering_size:              $macro_sink_cluster_size"
  puts "-num_static_layers:                  $num_static_layers"
  puts "-root_buf:                           $root_buffer"
  puts "-sink_buffer_max_cap_derate:         $sink_buffer_max_cap_derate"
  puts "-sink_clustering_levels:             $sink_clustering_levels"
  puts "-sink_clustering_max_diameter:       $sink_max_diameter"
  puts "-sink_clustering_size:               $sink_cluster_size"
  puts "-skip_nets:                          $skip_nets_list"
  puts "-tree_buf:                           $tree_buffer"
  puts "-wire_unit:                          $wire_segment_unit"
  puts "*****************************************************"
}

sta::define_cmd_args "reset_cts_config" {[-apply_ndr] \
                                         [-branching_point_buffers_distance] \
					 [-buf_list] \
                                         [-clock_buffer_footprint] \
                                         [-clock_buffer_string] \
                                         [-clustering_exponent] \
                                         [-clustering_unbalance_ratio] \
                                         [-delay_buffer_derate] \
                                         [-distance_between_buffers] \
                                         [-library] \
                                         [-macro_clustering_max_diameter] \
                                         [-macro_clustering_size] \
                                         [-num_static_layers] \
					 [-root_buf] \
                                         [-sink_buffer_max_cap_derate] \
                                         [-sink_clustering_levels] \
                                         [-sink_clustering_max_diameter] \
                                         [-sink_clustering_size] \
					 [-skip_nets] \
                                         [-tree_buf] \
	                                 [-wire_unit]}

proc reset_cts_config { args } {
  sta::parse_key_args "reset_cts_config" args \
    keys {} \
    flags {-apply_ndr -buf_list -branching_point_buffers_distance \
           -clock_buffer_footprint -clock_buffer_string \
           -clustering_exponent \
           -clustering_unbalance_ratio -delay_buffer_derate -distance_between_buffers \
           -library -macro_clustering_max_diameter -macro_clustering_size \
           -num_static_layers -root_buf -sink_buffer_max_cap_derate -sink_clustering_levels \
           -sink_clustering_max_diameter -sink_clustering_size -skip_nets -tree_buf -wire_unit}

  set reset_all [expr { [array size flags] == 0 }]

  if { $reset_all || [info exists flags(-apply_ndr)] } {
    cts::reset_apply_ndr
    utl::info CTS 211 "NDR strategy has been removed."
  }
  if { $reset_all || [info exists flags(-branching_point_buffers_distance)] } {
    cts::reset_branching_point_buffers_distance
    utl::info CTS 212 "Branch buffer distance has been removed."
  }
  if { $reset_all || [info exists flags(-buf_list)] } {
    cts::reset_buffer_list
    utl::info CTS 213 "Buffer list has been removed."
  }
  if {
    $reset_all || [info exists flags(-clock_buffer_string)]
    || [info exists flags(-clock_buffer_footprint)]
  } {
    rsz::reset_clock_buffer_pattern_cmd
  }
  if { $reset_all || [info exists flags(-clustering_exponent)] } {
    cts::reset_clustering_exponent
    utl::info CTS 214 "Clustering power has been removed."
  }
  if { $reset_all || [info exists flags(-clustering_unbalance_ratio)] } {
    cts::reset_clustering_unbalance_ratio
    utl::info CTS 215 "Clustering unbalance ratio has been removed."
  }
  if { $reset_all || [info exists flags(-delay_buffer_derate)] } {
    cts::reset_delay_buffer_derate
    utl::info CTS 216 "Delay buffer derate has been removed."
  }
  if { $reset_all || [info exists flags(-distance_between_buffers)] } {
    cts::reset_distance_between_buffers
    utl::info CTS 217 "Distance between buffers has been removed."
  }
  if { $reset_all || [info exists flags(-library)] } {
    cts::reset_cts_library
    utl::info CTS 218 "CTS library has been removed."
  }
  if { $reset_all || [info exists flags(-macro_clustering_max_diameter)] } {
    cts::reset_macro_clustering_diameter
    utl::info CTS 219 "Macro clustering max diameter has been removed."
  }
  if { $reset_all || [info exists flags(-macro_clustering_size)] } {
    cts::reset_macro_clustering_size
    utl::info CTS 220 "Macro clustering size has been removed."
  }
  if { $reset_all || [info exists flags(-num_static_layers)] } {
    cts::reset_num_static_layers
    utl::info CTS 221 "Number of static layers has been removed."
  }
  if { $reset_all || [info exists flags(-root_buf)] } {
    cts::reset_root_buffer
    utl::info CTS 222 "Root buffer has been removed."
  }
  if { $reset_all || [info exists flags(-sink_buffer_max_cap_derate)] } {
    cts::reset_sink_buffer_max_cap_derate
    utl::info CTS 223 "Sink buffer max cap derate has been removed."
  }
  if { $reset_all || [info exists flags(-sink_clustering_levels)] } {
    cts::reset_sink_clustering_levels
    utl::info CTS 224 "Sink clustering levels has been removed."
  }
  if { $reset_all || [info exists flags(-sink_clustering_max_diameter)] } {
    cts::reset_clustering_diameter
    utl::info CTS 225 "Sink clustering max diameter has been removed."
  }
  if { $reset_all || [info exists flags(-sink_clustering_size)] } {
    cts::reset_sink_clustering_size
    utl::info CTS 226 "Sink clustering size has been removed."
  }
  if { $reset_all || [info exists flags(-skip_nets)] } {
    cts::reset_skip_nets
    utl::info CTS 227 "Skip nets has been removed."
  }
  if { $reset_all || [info exists flags(-tree_buf)] } {
    cts::reset_tree_buf
    utl::info CTS 228 "Tree buffer has been removed."
  }
  if { $reset_all || [info exists flags(-wire_unit)] } {
    cts::reset_wire_segment_distance_unit
    utl::info CTS 229 "Wire segment unit has been removed."
  }
}

sta::define_cmd_args "clock_tree_synthesis" {[-wire_unit unit]
                                             [-buf_list buflist] \
                                             [-root_buf buf] \
                                             [-clk_nets nets] \
                                             [-tree_buf buf] \
                                             [-distance_between_buffers] \
                                             [-branching_point_buffers_distance] \
                                             [-clustering_exponent] \
                                             [-clustering_unbalance_ratio] \
                                             [-sink_clustering_size] \
                                             [-sink_clustering_max_diameter] \
                                             [-macro_clustering_size] \
                                             [-macro_clustering_max_diameter] \
                                             [-sink_clustering_enable] \
                                             [-balance_levels] \
                                             [-sink_clustering_levels levels] \
                                             [-num_static_layers] \
                                             [-sink_clustering_buffer] \
                                             [-obstruction_aware] \
                                             [-no_obstruction_aware] \
                                             [-apply_ndr strategy] \
                                             [-sink_buffer_max_cap_derate] \
                                             [-dont_use_dummy_load] \
                                             [-delay_buffer_derate] \
                                             [-library] \
                                             [-repair_clock_nets] \
                                             [-no_insertion_delay]
} ;# checker off

proc clock_tree_synthesis { args } {
  sta::parse_key_args "clock_tree_synthesis" args \
    keys {-root_buf -buf_list -wire_unit -clk_nets -sink_clustering_size \
          -num_static_layers -sink_clustering_buffer \
          -distance_between_buffers -branching_point_buffers_distance \
          -clustering_exponent \
          -clustering_unbalance_ratio -sink_clustering_max_diameter \
          -macro_clustering_size -macro_clustering_max_diameter \
          -sink_clustering_levels -tree_buf \
          -apply_ndr \
          -sink_buffer_max_cap_derate -delay_buffer_derate -library} \
    flags {-post_cts_disable -sink_clustering_enable -balance_levels \
           -obstruction_aware -no_obstruction_aware \
           -dont_use_dummy_load -repair_clock_nets -no_insertion_delay
  } ;# checker off

  sta::check_argc_eq0 "clock_tree_synthesis" $args

  if { [info exists keys(-library)] } {
    set cts_library $keys(-library)
    cts::set_cts_library $cts_library
  }

  if { [info exists flags(-post_cts_disable)] } {
    utl::warn CTS 115 "-post_cts_disable is obsolete."
  }

  cts::set_sink_clustering [info exists flags(-sink_clustering_enable)]

  if { [info exists keys(-sink_clustering_size)] } {
    set size $keys(-sink_clustering_size)
    cts::set_sink_clustering_size $size
  }

  if { [info exists keys(-sink_clustering_max_diameter)] } {
    set distance $keys(-sink_clustering_max_diameter)
    cts::set_clustering_diameter $distance
  }

  if { [info exists keys(-macro_clustering_size)] } {
    set size $keys(-macro_clustering_size)
    cts::set_macro_clustering_size $size
  }

  if { [info exists keys(-macro_clustering_max_diameter)] } {
    set distance $keys(-macro_clustering_max_diameter)
    cts::set_macro_clustering_diameter $distance
  }
  if { [info exists flags(-balance_levels)] } {
    utl::warn CTS 132 "-balance_levels is obsolete."
  }

  if { [info exists keys(-sink_clustering_levels)] } {
    set levels $keys(-sink_clustering_levels)
    cts::set_sink_clustering_levels $levels
  }

  if { [info exists keys(-num_static_layers)] } {
    set num $keys(-num_static_layers)
    cts::set_num_static_layers $num
  }

  if { [info exists keys(-distance_between_buffers)] } {
    set distance $keys(-distance_between_buffers)
    cts::set_distance_between_buffers [ord::microns_to_dbu $distance]
  }

  if { [info exists keys(-branching_point_buffers_distance)] } {
    set distance $keys(-branching_point_buffers_distance)
    cts::set_branching_point_buffers_distance [ord::microns_to_dbu $distance]
  }

  if { [info exists keys(-clustering_exponent)] } {
    set exponent $keys(-clustering_exponent)
    cts::set_clustering_exponent $exponent
  }

  if { [info exists keys(-clustering_unbalance_ratio)] } {
    set unbalance $keys(-clustering_unbalance_ratio)
    cts::set_clustering_unbalance_ratio $unbalance
  }

  if { [info exists keys(-buf_list)] } {
    set buf_list $keys(-buf_list)
    cts::set_buffer_list $buf_list
  } else {
    set buf_list [cts::get_buffer_list]
    if { $buf_list eq "" } {
      cts::set_buffer_list ""
    }
  }

  if { [info exists keys(-wire_unit)] } {
    set wire_unit $keys(-wire_unit)
    cts::set_wire_segment_distance_unit $wire_unit
  }

  if { [info exists keys(-clk_nets)] } {
    set clk_nets $keys(-clk_nets)
    set fail [cts::set_clock_nets $clk_nets]
    if { $fail } {
      utl::error CTS 56 "Error when finding -clk_nets in DB."
    }
  }

  if { [info exists keys(-tree_buf)] } {
    set buf $keys(-tree_buf)
    cts::set_tree_buf $buf
  }

  if { [info exists keys(-root_buf)] } {
    set root_buf $keys(-root_buf)
    cts::set_root_buffer $root_buf
  } else {
    set root_buf [cts::get_root_buffer]
    if { $root_buf eq "" } {
      cts::set_root_buffer ""
    }
  }

  if { [info exists keys(-sink_clustering_buffer)] } {
    set sink_buf $keys(-sink_clustering_buffer)
    cts::set_sink_buffer $sink_buf
  } else {
    cts::set_sink_buffer ""
  }

  if { [info exists keys(-sink_buffer_max_cap_derate)] } {
    set derate $keys(-sink_buffer_max_cap_derate)
    if { $derate > 1.0 || $derate < 0.0 } {
      utl::error CTS 109 "sink_buffer_max_cap_derate needs to be between 0 and 1.0."
    }
    cts::set_sink_buffer_max_cap_derate $derate
  }

  if { [info exists keys(-delay_buffer_derate)] } {
    set buffer_derate $keys(-delay_buffer_derate)
    if { $buffer_derate < 0.0 } {
      utl::error CTS 123 "delay_buffer_derate needs to be greater than or equal to 0."
    }
    cts::set_delay_buffer_derate $buffer_derate
  }

  if { [info exists flags(-obstruction_aware)] } {
    utl::warn CTS 128 "-obstruction_aware is obsolete."
  }

  if { [info exists flags(-no_obstruction_aware)] } {
    cts::set_obstruction_aware false
  }
  if { [info exists flags(-dont_use_dummy_load)] } {
    cts::set_dummy_load false
  } else {
    cts::set_dummy_load true
  }

  if { [info exists keys(-apply_ndr)] } {
    set strategy $keys(-apply_ndr)
    cts::set_apply_ndr $strategy
  }

  if { [info exists flags(-repair_clock_nets)] } {
    cts::set_repair_clock_nets true
  } else {
    cts::set_repair_clock_nets false
  }

  if { [info exists flags(-no_insertion_delay)] } {
    cts::set_insertion_delay false
  } else {
    cts::set_insertion_delay true
  }

  if { [ord::get_db_block] == "NULL" } {
    utl::error CTS 103 "No design block found."
  }
  cts::run_triton_cts
}

sta::define_cmd_args "report_cts" {[-out_file file] \
                                  }
proc report_cts { args } {
  sta::parse_key_args "report_cts" args \
    keys {-out_file} flags {}

  sta::check_argc_eq0 "report_cts" $args

  if { [info exists keys(-out_file)] } {
    set outFile $keys(-out_file)
    cts::set_metric_output $outFile
  }

  cts::report_cts_metrics
}

namespace eval cts {
proc clock_tree_synthesis_debug { args } {
  sta::parse_key_args "clock_tree_synthesis_debug" args \
    keys {} flags {-plot} ;# checker off

  sta::check_argc_eq0 "clock_tree_synthesis_debug" $args
  cts::set_plot_option [info exists flags(-plot)]

  cts::set_debug_cmd
}
}

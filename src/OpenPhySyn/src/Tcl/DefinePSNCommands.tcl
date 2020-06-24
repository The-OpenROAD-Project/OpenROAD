R"===<><>===(



namespace eval psn {
    proc define_cmd_args { cmd arglist } {
        sta::define_cmd_args $cmd $arglist
        namespace export $cmd
    }
    
    define_cmd_args "transform" {transform_name args}
    proc transform {transform_name args} {
        psn::transform_internal $transform_name $args
    }

    define_cmd_args "pin_swap" {\
        [-path_count count]\
        [-power]\
    }
    
    proc pin_swap { args } {
        sta::parse_key_args "pin_swap" args \
            keys {-path_count} \
            flags {-power}

        set max_path_count 50
        if {[info exists keys(-path_count)]} {
            set max_path_count $keys(-path_count)
        }
        set power_flag ""
        if {[info exists flags(-power)]} {
            set power_flag "-power"
        }
        set transform_args "$max_path_count $power_flag"
        set transform_args [string trim $transform_args]
        set num_swapped [transform pin_swap {*}$transform_args]
        return $num_swapped
    }
    
    define_cmd_args "optimize_logic" {\
        [-no_constant_propagation] \
        [-tiehi tiehi_cell_name] \
        [-tielo tielo_cell_name] \
    }

    proc optimize_logic { args } {
        sta::parse_key_args "optimize_power" args \
            keys {-tiehi tielo} \
            flags {-no_constant_propagation}
        set do_constant_propagation true
        set tiehi_cell_name ""
        set tielo_cell_name ""
        set max_prop_depth -1
        if {[info exists flags(-no_constant_propagation)]} {
            set no_constant_propagation false
        }
        if {![has_transform constant_propagation]} {
            set do_constant_propagation false
        }
        if {$do_constant_propagation} {
            if {[info exists keys(-tiehi)]} {
                set tiehi_cell_name $keys(-tiehi)
            }
            if {[info exists keys(-tielo)]} {
                set tielo_cell_name $keys(-tielo)
            }
            set propg [transform constant_propagation true $max_prop_depth $tiehi_cell_name $tielo_cell_name]
            if {$propg < 0} {
                return $propg
            }
        }
        return 0
    }

    define_cmd_args "optimize_power" {\
        [-no_pin_swap] \
        [-pin_swap_paths path_count] \
    }
    proc optimize_power { args } {
        sta::parse_key_args "optimize_power" args \
            keys {-pin_swap_paths} \
            flags {-no_pin_swap}

        set do_pin_swap true

        if {[info exists flags(-no_pin_swap)]} {
            set do_pin_swap false
        }
        if {![has_transform pin_swap]} {
            set do_pin_swap false
        }

        set num_swapped 0

        if {$do_pin_swap} {
            set pin_swap_paths 50
            if {[info exists keys(-pin_swap_paths)]} {
                set pin_swap_paths $keys(-pin_swap_paths)
            }
            set num_swapped [transform pin_swap true $pin_swap_paths]

            if {$num_swapped < 0} {
                return $num_swapped
            }
        }
        return 1
    }


    define_cmd_args "optimize_fanout" { \
        -buffer_cell buffer_cell_name \
        -max_fanout max_fanout \
    }

    proc optimize_fanout { args } {
        sta::parse_key_args "optimize_fanout" args \
            keys {-buffer_cell -max_fanout} \
            flags {}
        if { ![info exists keys(-buffer_cell)] \
          || ![info exists keys(-max_fanout)]
         } {
            sta::cmd_usage_error "optimize_fanout"
        }
        set cell $keys(-buffer_cell)
        set max_fanout $keys(-max_fanout)
        transform buffer_fanout $max_fanout $cell
    }

    define_cmd_args "cluster_buffers" {[-cluster_threshold diameter] [-cluster_size single|small|medium|large|all]}
    proc cluster_buffers { args } {
        sta::parse_key_args "cluster_buffers" args \
        keys {-cluster_threshold -cluster_size} \
        flags {}

        if {![psn::has_liberty]} {
            sta::sta_error "No liberty filed is loaded"
            return
        }

        if {[info exists keys(-cluster_threshold)] && [info exists keys(-cluster_size)]} {
            sta::sta_error "You can only specify either -cluster_threshold or -cluster_size"
            return
        } elseif {![info exists keys(-cluster_threshold)] && ![info exists keys(-cluster_size)]} {
            sta::cmd_usage_error "cluster_buffers"
            return
        }
        if {[info exists keys(-cluster_threshold)]} {
            if {($keys(-cluster_threshold) < 0.0) || ($keys(-cluster_threshold) > 1.0)} {
                sta::sta_error "-cluster_threshold should be between 0.0 and 1.0"
                return
            }
            return [psn::cluster_buffer_names $keys(-cluster_threshold) true]
        } else {
            set size $keys(-cluster_size)
            set valid_lib_size [list "single" "small" "medium" "large" "all"]
            if {[lsearch -exact $valid_lib_size $size] < 0} {
                sta::sta_error "Invalid value for -cluster_size, valid values are $valid_lib_size"
                return
            }
            set cluster_threshold ""
            if {$size == "single"} {
                set cluster_threshold 1.0
            } elseif {$size == "small"} {
                set cluster_threshold [expr 3.0 / 4.0]
            } elseif {$size == "medium"} {
                set cluster_threshold [expr 1.0 / 4.0]
            } elseif {$size == "large"} {
                set cluster_threshold [expr 1.0 / 12.0]
            } elseif {$size == "all"} {
                set cluster_threshold 0.0
            }

            return [psn::cluster_buffer_names $cluster_threshold true]
        }
    }

    define_cmd_args "timing_buffer" {[-capacitance_violations] [-transition_violations] [-negative_slack]\
				 [-timerless] [-repair_by_resize] \
				 [-auto_buffer_library single|small|medium|large|all]\
				 [-minimize_buffer_library] [-high_effort]\
				 [-use_inverting_buffer_library] [-buffers buffers]\
				 [-inverters inverters ] [-iterations iterations] [-area_penalty area_penalty]\
				 [-legalization_frequency count] [-minimum_gain gain] [-enable_driver_resize]\
    }

    
    proc timing_buffer { args } {
        sta::parse_key_args "timing_buffer" args \
        keys {-auto_buffer_library -buffers -inverters -iterations -minimum_gain -area_penalty -legalization_frequency}\
        flags {-negative_slack_violations -timerless -capacitance_violations -transition_violations -repair_by_resize -high_effort -repair_by_resynthesis -enable_driver_resize -minimize_buffer_library -use_inverting_buffer_library -capacitance_violations] -transition_violations}
        
        set buffer_lib_flag ""
        set auto_buf_flag ""
        set mode_flag ""

        set has_max_cap [info exists flags(-capacitance_violations)]
        set has_max_transition [info exists flags(-transition_violations)]
        set has_max_ns [info exists flags(-negative_slack_violations)]

        set repair_target_flag ""

        if {$has_max_cap} {
            set repair_target_flag "-capacitance_violations"
        }
        if {$has_max_transition} {
            set repair_target_flag "$repair_target_flag -transition_violations"
        }
        if {$has_max_ns} {
            set repair_target_flag "$repair_target_flag -negative_slack_violations"
        }
        if {[info exists flags(-repair_by_resize)]} {
            set repair_target_flag "$repair_target_flag -repair_by_resize"
        }

        if {[info exists flags(-repair_by_resynthesis)]} {
            set repair_target_flag "$repair_target_flag -repair_by_resynthesis"
        }

        if {[info exists flags(-timerless)]} {
            set mode_flag "-timerless"
        }
        if {[info exists flags(-cirtical_path)]} {
            set mode_flag "$mode_flag -cirtical_path"
        }

        set fast_mode_flag ""
        if {[info exists flags(-high_effort)]} {
            set fast_mode_flag "-high_effort"
        }
        

        if {[info exists flags(-maximize_slack)]} {
            if {[info exists flags(-timerless)]} {
                 sta::sta_error "Cannot use -maximize_slack with -timerless mode"
                retrun
            }
            set mode_flag "$mode_flag -maximize_slack"
        }

        set has_auto_buff [info exists keys(-auto_buffer_library)]

        if {![psn::has_liberty]} {
            sta::sta_error "No liberty filed is loaded"
            return
        }

        if {$has_auto_buff} {
            set valid_lib_size [list "single" "small" "medium" "large" "all"]
            set buffer_lib_size $keys(-auto_buffer_library)
            if {[lsearch -exact $valid_lib_size $buffer_lib_size] < 0} {
                sta::sta_error "Invalid value for -auto_buffer_library, valid values are $valid_lib_size"
                return
            }
            set auto_buf_flag "-auto_buffer_library $buffer_lib_size"
        }

        if {[info exists keys(-buffers)]} {
            set blist $keys(-buffers)
            set buffer_lib_flag "-buffers $blist"
        }
        set inverters_flag ""
        if {[info exists keys(-inverters)]} {
            set ilist $keys(-inverters)
            set inverters_flag "-inverters $ilist"
        }
        set minimuze_buf_lib_flag ""
        if {[info exists flags(-minimize_buffer_library)]} {
            if {!$has_auto_buff} {
                sta::sta_error "-minimize_buffer_library can only be used with -auto_buffer_library"
                return
            }
            set minimuze_buf_lib_flag  "-minimize_buffer_library"
        }
        set use_inv_buf_lib_flag ""
        if {[info exists flags(-use_inverting_buffer_library)]} {
            if {!$has_auto_buff} {
                sta::sta_error "-use_inverting_buffer_library can only be used with -auto_buffer_library"
                return
            }
            set use_inv_buf_lib_flag  "-use_inverting_buffer_library"
        }
        set minimum_gain_flag ""
        if {[info exists keys(-minimum_gain)]} {
            set minimum_gain_flag  "-minimum_gain $keys(-minimum_gain)"
        }
        set legalization_freq_flag ""
        if {[info exists keys(-legalization_frequency)]} {
            set legalization_freq_flag  "-legalization_frequency $keys(-legalization_frequency)"
        }
        set area_penalty_flag ""
        if {[info exists keys(-area_penalty)]} {
            set area_penalty_flag  "-area_penalty $keys(-area_penalty)"
        }
        set resize_flag ""
        if {[info exists flags(-enable_driver_resize)]} {
            set resize_flag  "-enable_driver_resize"
        }
        set iterations 1
        if {[info exists keys(-iterations)]} {
            set iterations "$keys(-iterations)"
        }
        set bufargs "$repair_target_flag $fast_mode_flag $mode_flag $auto_buf_flag $minimuze_buf_lib_flag $use_inv_buf_lib_flag $legalization_freq_flag $buffer_lib_flag $inverters_flag $minimum_gain_flag $resize_flag $area_penalty_flag -iterations $iterations"
        set affected [transform timing_buffer {*}$bufargs]
        if {$affected < 0} {
            puts "Timing buffer failed"
            return
        }
        puts "Added/updated $affected cells"
    }


    define_cmd_args "repair_timing" {[-capacitance_violations]\
        [-transition_violations]\
        [-negative_slack_violations] [-iterations iteration_count] [-buffers buffer_cells]\
        [-inverters inverter cells] [-minimum_gain gain] [-auto_buffer_library size]\
        [-no_minimize_buffer_library] [-auto_buffer_library_inverters_enabled]\
        [-buffer_disabled] [-minimum_cost_buffer_enabled] [-resize_disabled]\
        [-downsize_enabled] [-pin_swap_disabled] [-legalize_eventually]\
        [-legalize_each_iteration] [-post_place] [-post_route] [-pins pin_names] [-no_resize_for_negative_slack]\
        [-legalization_frequency num_edits] [-high_effort] [-capacitance_pessimism_factor factor] [-transition_pessimism_factor factor]\
        [-upstream_resistance res] [-maximum_negative_slack_paths count] [-maximum_negative_slack_path_depth count]\
    }

    proc repair_timing { args } {
        if {![psn::has_liberty]} {
            sta::sta_error "No liberty filed is loaded"
            return
        }
        
        
        set affected [transform repair_timing {*}$args]
        if {$affected < 0} {
             puts "Timing repair failed"
             return
        }
        puts "Added/updated $affected cells"
    }

    define_cmd_args "gate_clone" {\
        [-clone_max_cap_factor factor] \
        [-clone_non_largest_cells] \
    }
    
    proc gate_clone { args } {
        sta::parse_key_args "gate_clone" args \
        keys {-clone_max_cap_factor} \
        flags {-clone_non_largest_cells}

        set clone_max_cap_factor 1.5
        set clone_largest_cells_only true

        if {[info exists keys(-clone_max_cap_factor)]} {
            set clone_max_cap_factor $keys(-clone_max_cap_factor)
        }
        if {[info exists flags(-clone_non_largest_cells)]} {
            set clone_largest_cells_only false
        }
        set transform_args "$clone_max_cap_factor $clone_largest_cells_only"
        set transform_args [string trim $transform_args]
        set num_cloned [transform gate_clone {*}$transform_args]
        return $num_cloned
    }


    define_cmd_args "propagate_constants" {[-tiehi tiehi_cell] [-tielo tielo_cell] [-inverter inverter_cell]}

    proc propagate_constants { args } {
        sta::parse_key_args "cluster_buffers" args \
        keys {-tiehi -tielo -inverter} \
        flags {}

        if {![psn::has_liberty]} {
            sta::sta_error "No liberty filed is loaded"
            return
        }
        
        set transform_args "-1"
        if {[info exists keys(-tiehi)]} {
            set transform_args "$transform_args $keys(-tiehi)"
            if {[info exists keys(-tielo)]} {
                set transform_args "$transform_args $keys(-tielo)"
                if {[info exists keys(-inverter)]} {
                    set transform_args "$transform_args $keys(-inverter)"
                }
            }
        }
        return [transform constant_propagation {*}$transform_args]
    }

    define_cmd_args "set_psn_dont_use" {lib_cells}

    proc set_psn_dont_use { args } {
        psn::set_dont_use {*}$args
    }
}
namespace import psn::*

)===<><>==="
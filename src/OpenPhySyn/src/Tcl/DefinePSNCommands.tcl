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

    define_cmd_args "optimize_design" {\
        [-no_gate_clone] \
        [-no_pin_swap] \
        [-clone_max_cap_factor factor] \
        [-clone_non_largest_cells] \
    }
    
    proc optimize_design { args } {
        sta::parse_key_args "optimize_design" args \
            keys {-clone_max_cap_factor} \
            flags {-clone_non_largest_cells -no_gate_clone -no_pin_swap}

        set do_pin_swap true
        set do_gate_clone true
        if {[info exists flags(-no_gate_clone)]} {
            set do_gate_clone false
        }
        if {[info exists flags(-no_pin_swap)]} {
            set do_pin_swap false
        }
        if {![has_transform gate_clone]} {
            set do_gate_clone false
        }
        if {![has_transform pin_swap]} {
            set do_pin_swap false
        }
        set num_swapped 0
        set num_cloned 0
        if {$do_pin_swap} {
            set num_swapped [transform pin_swap]
            if {$num_swapped < 0} {
                return $num_swapped
            }
        }
        if {$do_gate_clone} {
            set clone_max_cap_factor 1.5
            set clone_largest_cells_only true

            if {[info exists keys(-clone_max_cap_factor)]} {
                set clone_max_cap_factor $keys(-clone_max_cap_factor)
            }
            if {[info exists flags(-clone_non_largest_cells)]} {
                set clone_largest_cells_only false
            }

            set num_cloned [transform gate_clone $clone_max_cap_factor $clone_largest_cells_only]
            if {$num_cloned < 0} {
                return $num_cloned
            }
        }
        return 1
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
}
namespace import psn::*

)===<><>==="
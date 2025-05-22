# Helper function to split a string into a list of strings and numbers
proc split_strings_and_numbers {str} {
    set result {}
    foreach {all letters numbers} [regexp -all -inline {(\D*)(\d*)} $str] {
        if {$letters ne ""} {
            lappend result $letters
        }
        if {$numbers ne ""} {
            lappend result [expr {$numbers + 0}]  ;# Convert to integer
        }
    }
    return $result
}

# Custom comparison function
proc natural_compare {str1 str2} {
    set list1 [split_strings_and_numbers $str1]
    set list2 [split_strings_and_numbers $str2]
    set len [expr {min([llength $list1], [llength $list2])}]
    for {set i 0} {$i < $len} {incr i} {
        set part1 [lindex $list1 $i]
        set part2 [lindex $list2 $i]
        if {$part1 ne $part2} {
            if {[string is integer -strict $part1] && [string is integer -strict $part2]} {
                return [expr {$part1 - $part2}]
            } else {
                return [string compare $part1 $part2]
            }
        }
    }
    return [expr {[llength $list1] - [llength $list2]}]  ;# If all parts are equal, compare by length
}

proc natural_sort {list} {
    return [lsort -command natural_compare $list]
}

proc match_pins { regex {direction .*} {is_clock 0}} {
    set pins {}
    # The regex for get_ports is not the tcl regex
    foreach pin [get_ports -regex .*] {
        set input [get_property $pin name]
        # We want the Tcl regex
        if {![regexp $regex $input]} {
            continue
        }
        if {![regexp $direction [get_property $pin direction]]} {
            continue
        }
        if {[expr $is_clock != [sta::is_clock_src [sta::get_port_pin $pin]]]} {
            continue
        }
        lappend pins [get_property $pin name]
    }
    return [natural_sort $pins]
}

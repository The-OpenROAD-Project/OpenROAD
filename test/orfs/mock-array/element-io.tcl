source test/orfs/mock-array/util.tcl

set assignments [list \
    top bottom \
    [list [ concat \
        {*}[match_pins io_ins_down.*] \
        {*}[match_pins io_outs_up.*] \
    ] \
    [ concat \
        {*}[match_pins io_outs_down.*] \
        {*}[match_pins io_ins_up.*] \
    ]] \
    left right \
    [list [ concat \
        {*}[match_pins io_ins_right.*] \
        {*}[match_pins io_outs_left.*] \
    ] \
    [ concat \
        {*}[match_pins io_outs_right.*] \
        {*}[match_pins io_ins_left.*] \
    ]] \
    left right \
    [list [ concat \
        {*}[match_pins io_lsbIns_.*] \
    ] \
    [ concat \
        {*}[match_pins io_lsbOuts_.*] \
    ]]
]

proc zip {list1 list2} {
    set result {}
    set length [llength $list1]
    set skip [expr [llength $list2] - [llength $list1]]
    for {set i 0} {$i < $length} {incr i} {
        lappend result [lindex $list2 [expr $skip + $i]] [lindex $list1 $i]
    }
    return $result
}


foreach {direction direction2 names} $assignments {
    set mirrored [zip {*}$names]
    set_io_pin_constraint -region $direction2:* -pin_names [lindex $names 1]
    # Test pins across multiple metal layers; so don't group
    # pins as a group of pins must be on a single metal layer.
    #
    # set_io_pin_constraint -group -order -pin_names [lindex $names 1]
    set_io_pin_constraint -mirrored_pins $mirrored
}

set_io_pin_constraint -region top:* -pin_names clock

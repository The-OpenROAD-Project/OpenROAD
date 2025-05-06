source test/orfs/mock-array/util.tcl

set assignments [list \
    top \
    [ concat \
        {*}[match_pins io_ins_down_.*] \
        {*}[match_pins io_outs_up_.*] \
    ] \
    bottom \
    [ concat \
        {*}[match_pins io_ins_up_.*] \
        {*}[match_pins io_outs_down_.*] \
    ] \
    left \
    [ concat \
        {*}[match_pins io_ins_right_.*] \
        {*}[match_pins io_outs_left_.*] \
    ] \
    right \
    [ concat \
        {*}[match_pins io_ins_left_.*] \
        {*}[match_pins io_outs_right_.*] \
        {*}[match_pins io_lsbs_.*] \
    ] \
]

foreach {direction names} $assignments {
    set_io_pin_constraint -region $direction:* -pin_names $names
}

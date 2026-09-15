source test/orfs/mock-array/util.tcl

set assignments [list \
  top \
  [concat \
    {*}[match_pins io_ins_down.*] \
    {*}[match_pins io_outs_up.*]] \
  bottom \
  [concat \
    {*}[match_pins io_ins_up.*] \
    {*}[match_pins io_outs_down.*]] \
  left \
  [concat \
    {*}[match_pins io_ins_right.*] \
    {*}[match_pins io_outs_left.*]] \
  right \
  [concat \
    {*}[match_pins io_ins_left.*] \
    {*}[match_pins io_outs_right.*] \
    {*}[match_pins io_lsbs.*]]]

foreach {direction names} $assignments {
  set_io_pin_constraint -region $direction:* -pin_names $names
}

# set_wire_rc chip selector misuse errors
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def reg3.def

# unknown chip name is an error
catch { set_wire_rc -chip no_such_chip -resistance 1e-3 -capacitance 1e-1 } msg
puts $msg
# selectors are mutually exclusive
catch { set_wire_rc -tech Nangate45 -chip chip1 -resistance 1e-3 -capacitance 1e-1 } msg
puts $msg

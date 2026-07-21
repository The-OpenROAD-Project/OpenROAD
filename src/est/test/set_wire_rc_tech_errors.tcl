# set_wire_rc technology selector misuse errors
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def reg3.def

# unknown technology name is an error
catch { set_wire_rc -tech no_such_tech -resistance 1e-3 -capacitance 1e-1 } msg
puts $msg
# selectors are mutually exclusive
catch {
  set_wire_rc -tech Nangate45 -redistribution_layer \
    -resistance 1e-3 -capacitance 1e-1
} msg
puts $msg

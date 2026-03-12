# set_layer_rc
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg3.def

set_wire_rc -layer metal3
set corner [sta::cmd_scene]
# ohm/meter -> kohm/micron
set r [expr { [rsz::wire_signal_resistance $corner] * 1e-3 * 1e-6 }]
# F/meter -> fF/micron
set c [expr { [rsz::wire_signal_capacitance $corner] * 1e+15 * 1e-6 }]
puts "r=[format %.2e $r] c=[format %.2e $c]"
set_layer_rc -layer metal3 -resistance $r -capacitance $c

# This should give the same results
set_wire_rc -layer metal3
set r [expr { [rsz::wire_signal_resistance $corner] * 1e-3 * 1e-6 }]
# F/meter -> fF/micron
set c [expr { [rsz::wire_signal_capacitance $corner] * 1e+15 * 1e-6 }]
puts "r=[format %.2e $r] c=[format %.2e $c]"

set_layer_rc -via via1 -resistance .1

# non-routing layer
catch { set_layer_rc -layer via1 -resistance .1 -capacitance .1 } result

# via missing -resistance
# via -capacitance not supported
catch { set_layer_rc -via via1 -capacitance .1 } result

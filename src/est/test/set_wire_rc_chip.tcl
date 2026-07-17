# set_wire_rc -tech/-chip/-redistribution_layer chip selectors
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

# unknown chip name is an error
catch { set_wire_rc -chip no_such_chip -resistance 1e-3 -capacitance 1e-1 } msg
puts $msg
# selectors are mutually exclusive
catch { set_wire_rc -tech Nangate45 -chip chip1 -resistance 1e-3 -capacitance 1e-1 } msg
puts $msg
# selectors matching no chip warn and are ignored
set_wire_rc -tech no_such_tech -resistance 1e-3 -capacitance 1e-1
set_wire_rc -redistribution_layer -resistance 1e-3 -capacitance 1e-1

# a second tech makes the database multi-tech; tech-targeted values still work
read_lef -tech -tech_name tech2 Nangate45/Nangate45.lef

# a second chip on its own technology: -chip, -tech and RDL selectors succeed
set db [ord::get_db]
odb::dbChip_create $db [$db findTech tech2] "chip2" "RDL"
set_wire_rc -chip chip2 -resistance 1e-3 -capacitance 1e-1
set_wire_rc -tech tech2 -layer metal3
set_wire_rc -redistribution_layer -resistance 2e-3 -capacitance 2e-1

# heterogeneous RDL technologies cannot resolve a shared layer
odb::dbChip_create $db [$db findTech Nangate45] "chip3" "RDL"
set_wire_rc -redistribution_layer -resistance 2e-3 -capacitance 2e-1
catch { set_wire_rc -redistribution_layer -layer metal3 } msg
puts $msg

# same wire RC values as make_parasitics1
set lambda .12
# kohm/square.
set m1_res_sq .08e-3
# ff/micron^2
set m1_area_cap 39e-3
# ff/micron.
set m1_edge_cap 57e-3
# 4 lambda wide wire
set wire_cap [expr { $m1_area_cap * $lambda * 4 + $m1_edge_cap * 2 }]
set wire_res [expr { $m1_res_sq / ($lambda * 4) }]
set_wire_rc -tech Nangate45 -resistance $wire_res -capacitance $wire_cap
estimate_parasitics -placement

report_checks

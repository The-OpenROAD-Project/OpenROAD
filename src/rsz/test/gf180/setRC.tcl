# OpenRCX  - RC file for OpenROAD FS
# Platform - GF180 gf180mcu-pdk
#
# NOTE: LEF contains RC values per layer

# Tech LEF has the same unit RC for all layers
# CAPACITANCE CPERSQDIST 0.0000394 ;
# RESISTANCE RPERSQ 0.090000

# These values are from five gf180 designs: ibex, jpeg, aes-hybrid, riscv32i, aes
set_layer_rc -layer Metal2 -resistance 3.85861E-04 -capacitance 1.35357E-04
set_layer_rc -layer Metal3 -resistance 2.06673E-04 -capacitance 1.46141E-04
set_layer_rc -layer Metal4 -resistance 1.68609E-04 -capacitance 1.50688E-04
set_layer_rc -layer Metal5 -resistance 7.92778E-05 -capacitance 1.55595E-04
#set_wire_rc -resistance 2.35501E-04 -capacitance 1.42149E-04

regexp {(\d+)} $::env(METAL_OPTION) metal

if { $metal == "6" } {

  set_wire_rc -signal -layer Metal2
  set_wire_rc -clock  -layer Metal5

} elseif  { $metal == "5" } {
  # TC matches LEF.  These are the temperature adjusted values.
  # The other stacks are likely similar but I haven't checked yet.
  if {$::env(CORNER) == "WC"} {
    set_layer_rc -via Via1 -resistance 16.845
    set_layer_rc -via Via2 -resistance 16.845
    set_layer_rc -via Via3 -resistance 16.845
    set_layer_rc -via Via4 -resistance 16.845

    set tech [ord::get_db_tech]
    foreach via [$tech getVias] {
      if {[$via getResistance] == 4.5} {
        $via setResistance 16.845
      }
    }
  } elseif {$::env(CORNER) == "BC"} {
    set_layer_rc -via Via1 -resistance 4.23
    set_layer_rc -via Via2 -resistance 4.23
    set_layer_rc -via Via3 -resistance 4.23
    set_layer_rc -via Via4 -resistance 4.23

    set tech [ord::get_db_tech]
    foreach via [$tech getVias] {
      if {[$via getResistance] == 4.5} {
        $via setResistance 4.23
      }
    }
  }
  
  set_wire_rc -signal -layer Metal2
  set_wire_rc -clock  -layer Metal4

} elseif  { $metal == "4" } {

  set_wire_rc -signal -layer Metal2
  set_wire_rc -clock  -layer Metal3

} elseif  { $metal == "3" } {

  set_wire_rc -signal -layer Metal2
  set_wire_rc -clock  -layer Metal2

} elseif  { $metal == "2" } {

  set_wire_rc -signal -layer Metal2
  set_wire_rc -clock  -layer Metal2
}

# A hierarchical netlist with its supply pins wired through the ports of a
# module, under other names than one level up: rail is vdd_main, well is
# vbias, gnd is vss.  The library's cells have a well pin (VPB) next to the
# power pin (VPWR), so the register's power pins are on two nets; it still
# moves, because every cell retime creates has those pins.  The new register
# is connected pin by pin inside the module like the one it replaces, so the
# netlist written with its supply pins still reads rail, well and gnd there.
# read_verilog does not type the three nets as supplies, which changes
# nothing.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef retime_supply_bias.lef
read_liberty retime_supply_bias.lib
read_verilog retime_supply_hier.v
link_design -hier top
create_clock -period 0.16 clk

retime
check_axioms
set v_file [make_result_file retime_supply_hier.v]
write_verilog -include_pwr_gnd $v_file
set stream [open $v_file r]
foreach line [split [read $stream] "\n"] {
  if { [string match "*BDFF_X1*" $line] } {
    set in_register 1
  }
  if { [info exists in_register] } {
    puts [string trim $line]
    if { [string match "*;" $line] } {
      unset in_register
    }
  }
}
close $stream

# A register with a second pin marked as a clock stays: what rebuilds a
# register tells its clock pin from its other pins by the mark, and would
# put the enable of this one on the clock net.  The enable pin of the
# sky130hd enable flop is marked as LEF USE CLOCK would mark it; the four
# enable registers of retime_dsec_enable keep their nets and nothing behind
# them moves.
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_enable.v
link_design top
create_clock -period 0.75 clk
set enable_flop [[ord::get_db] findMaster sky130_fd_sc_hd__edfxtp_1]
[$enable_flop findMTerm DE] setSigType CLOCK

retime
foreach inst [[ord::get_db_block] getInsts] {
  if { [$inst getMaster] == $enable_flop } {
    set clock [[[$inst findITerm CLK] getNet] getName]
    set enable [[[$inst findITerm DE] getNet] getName]
    puts "[$inst getName]: CLK on $clock, DE on $enable"
  }
}

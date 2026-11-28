# A register with an input on a net with two drivers stays, on a failing
# path too: a move would put a new load on that net, and OpenSTA's delay
# calculation of a net with two drivers does not take one.  The enable net
# en of the enable register r1 has the drivers g_e and g_x: r1 stays, and
# the gate behind it is pinned (7 pinned gates).  With g_x moved to a net of
# its own, en has one driver: 6 pinned gates, and r1 moves, the new register
# enabled by en.  The result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_shared_enable.v
link_design top
create_clock -period 0.6 clk
set block [ord::get_db_block]
set enable_flop [[ord::get_db] findMaster sky130_fd_sc_hd__edfxtp_1]

proc report_enable_registers { } {
  foreach inst [[ord::get_db_block] getInsts] {
    if { [$inst getMaster] == $::enable_flop } {
      set data [[[$inst findITerm D] getNet] getName]
      set enable [[[$inst findITerm DE] getNet] getName]
      puts "[$inst getName]: D on $data, DE on $enable"
    }
  }
}

retime
report_enable_registers

set g_x_out [[$block findInst g_x] findITerm X]
$g_x_out disconnect
$g_x_out connect [odb::dbNet_create $block fx]
write_verilog_for_dsec retime_dsec_shared_enable before
retime
report_enable_registers
run_dsec_test retime_dsec_shared_enable \
  -liberty_files sky130hd/sky130hd_tt.lib -clock clk -resetn rst_n

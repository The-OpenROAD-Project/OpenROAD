# A register clocked by an expression of several pins ("CK & EN") never
# moves; the same cone with a DFF_X1 in its place retimes.  The result is
# proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty retime_gated_clock_expr.lib
read_lef Nangate45/Nangate45.lef
read_lef retime_gated_clock_expr.lef
read_verilog retime_gated_clock_expr.v
link_design top
initialize_floorplan -die_area "0 0 100 20" \
  -core_area "0 0 100 20" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
create_clock -period 0.1 clk

# Each cone in its own row, in path order, 5 microns apart.
proc place_row { names y } {
  set x 5
  foreach name $names {
    set inst [[ord::get_db_block] findInst $name]
    $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu $y]
    $inst setPlacementStatus PLACED
    incr x 5
  }
}
place_row {g0 r1 g1 g2 g3 g4 g5 r2 g6} 2.8
place_row {g_r g_e g7 rg g8 g9 g10 g11 g12 r3 g13} 8.4
set_wire_rc -resistance 1.0e-03 -capacitance 1.0e-05
estimate_parasitics -placement

dict set dsec_models DFFG_X1 {{D CK EN Q} {
 input D, CK, EN; output Q; wire d; reg Q;
 DSEC_MUX2 m (.A0(Q), .A1(D), .S(EN), .Y(d));
 always @(posedge CK) Q <= d;}}

write_verilog_for_dsec retime_gated_clock_expr before
retime
run_dsec_test retime_gated_clock_expr \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n

set v_file [make_result_file retime_gated_clock_expr.v]
write_verilog $v_file
diff_files retime_gated_clock_expr.vok $v_file

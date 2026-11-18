# report_vt_usage binding smoke test; TestResizer checks the counts.
source "helpers.tcl"

set verilog_file [make_result_file report_vt_usage.v]
set stream [open $verilog_file w]
puts $stream {
module inner (in, out);
  input in;
  output out;
  wire n0;
  BUFx2_ASAP7_75t_SL b2 (.A(in), .Y(n0));
  INVx1_ASAP7_75t_SL i1 (.A(n0), .Y(out));
endmodule
module sub (in, out);
  input in;
  output out;
  wire n0;
  BUFx2_ASAP7_75t_L b1 (.A(in), .Y(n0));
  inner u_inner (.in(n0), .out(out));
endmodule
module top (in, out);
  input in;
  output out;
  wire n0;
  BUFx2_ASAP7_75t_R b0 (.A(in), .Y(n0));
  sub u_sub (.in(n0), .out(out));
endmodule
}
close $stream

read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_LVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_SLVT_FF_nldm_220122.lib.gz
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_verilog $verilog_file
link_design top -hier

check "report_vt_usage" { catch { report_vt_usage } } 0
check "report_vt_usage -verbose" { catch { report_vt_usage -verbose } } 0
check "report_vt_usage -vt_order" \
  { catch { report_vt_usage -vt_order {SL L R} } } 0
check "report_vt_usage -vt_order with an unknown name" \
  { catch { report_vt_usage -vt_order {SLVT L} } } 0
check "report_vt_usage u_sub" { catch { report_vt_usage u_sub } } 0
check "report_vt_usage -verbose u_sub/u_inner" \
  { catch { report_vt_usage -verbose u_sub/u_inner } } 0
check_error "unknown module instance" { report_vt_usage no_such_inst } RSZ-3280
exit_summary

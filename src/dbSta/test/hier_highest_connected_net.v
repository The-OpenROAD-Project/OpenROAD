// 2-level hier reg-to-reg design. midnet/intermediate span sub_inst
// boundary so leaf iterms on those nets get both flat dbNet and
// dbModNet wrappers after link_design -hier. Clock buffer ck_buf
// isolates the top clk BTerm so driver iteration doesn't hit the
// top-BTerm bypass branch in Parasitics::findParasiticNet (which is
// independent of PR #3194 and would otherwise leave clk unannotated).

module sub (din, sub_clk, dout);
  input  din;
  input  sub_clk;
  output dout;

  wire q_int;

  DFF_X1 ff_sub (.D(din), .CK(sub_clk), .Q(q_int));
  BUF_X1 buf_sub (.A(q_int), .Z(dout));
endmodule

module top (in, clk, out);
  input  in;
  input  clk;
  output out;

  wire clk_int;
  wire midnet;
  wire intermediate;

  BUF_X1 ck_buf (.A(clk), .Z(clk_int));
  DFF_X1 ff_top (.D(in), .CK(clk_int), .Q(midnet));
  sub sub_inst (.din(midnet), .sub_clk(clk_int), .dout(intermediate));
  DFF_X1 ff_out (.D(intermediate), .CK(clk_int), .Q(out));
endmodule

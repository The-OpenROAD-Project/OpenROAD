// Test design for buffer clamping to core area.
// 6 loads (just over max_fanout 5) at die corners forces both a fanout
// buffer and a wire repeater whose Steiner points land outside the core.
module outside_core (clk, in1, out0, out1, out2, out3, out4, out5);
  input clk, in1;
  output out0, out1, out2, out3, out4, out5;

  wire net0;

  DFF_X1 drvr (.D(in1), .CK(clk), .Q(net0), .QN());
  DFF_X1 load0 (.D(net0), .CK(clk), .Q(out0), .QN());
  DFF_X1 load1 (.D(net0), .CK(clk), .Q(out1), .QN());
  DFF_X1 load2 (.D(net0), .CK(clk), .Q(out2), .QN());
  DFF_X1 load3 (.D(net0), .CK(clk), .Q(out3), .QN());
  DFF_X1 load4 (.D(net0), .CK(clk), .Q(out4), .QN());
  DFF_X1 load5 (.D(net0), .CK(clk), .Q(out5), .QN());
endmodule

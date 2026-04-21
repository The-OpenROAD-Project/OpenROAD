// Test design for buffer clamping to core area.
// High fanout net with long wires to trigger wire repeater insertion.
module outside_core (clk, in1, out0, out1, out2, out3, out4,
                     out5, out6, out7, out8, out9);
  input clk, in1;
  output out0, out1, out2, out3, out4, out5, out6, out7, out8, out9;

  wire d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
  wire net0;

  DFF_X1 drvr (.D(in1), .CK(clk), .Q(net0), .QN());
  DFF_X1 load0 (.D(net0), .CK(clk), .Q(out0), .QN());
  DFF_X1 load1 (.D(net0), .CK(clk), .Q(out1), .QN());
  DFF_X1 load2 (.D(net0), .CK(clk), .Q(out2), .QN());
  DFF_X1 load3 (.D(net0), .CK(clk), .Q(out3), .QN());
  DFF_X1 load4 (.D(net0), .CK(clk), .Q(out4), .QN());
  DFF_X1 load5 (.D(net0), .CK(clk), .Q(out5), .QN());
  DFF_X1 load6 (.D(net0), .CK(clk), .Q(out6), .QN());
  DFF_X1 load7 (.D(net0), .CK(clk), .Q(out7), .QN());
  DFF_X1 load8 (.D(net0), .CK(clk), .Q(out8), .QN());
  DFF_X1 load9 (.D(net0), .CK(clk), .Q(out9), .QN());
endmodule

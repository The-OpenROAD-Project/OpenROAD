module bank (clk, d, qn);
  input clk;
  input [1:0] d;
  output [1:0] qn;

  DFFHQNx1_ASAP7_75t_L ff_a (.CLK(clk), .D(d[0]), .QN(qn[0]));
  DFFHQNx1_ASAP7_75t_L ff_b (.CLK(clk), .D(d[1]), .QN(qn[1]));
endmodule

module tray_test (clk1, d1, d2, d3, d4, o1, o2, o3, o4);
  input  clk1;
  input  d1;
  input  d2;
  input  d3;
  input  d4;
  output o1;
  output o2;
  output o3;
  output o4;

  bank bank0 (.clk(clk1), .d({d2, d1}), .qn({o2, o1}));
  bank bank1 (.clk(clk1), .d({d4, d3}), .qn({o4, o3}));
endmodule

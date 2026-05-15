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

  DFFHQNx1_ASAP7_75t_L ff1 (.CLK(clk1), .D(d1), .QN(o1));
  DFFHQNx1_ASAP7_75t_L ff2 (.CLK(clk1), .D(d2), .QN(o2));
  DFFHQNx1_ASAP7_75t_L ff3 (.CLK(clk1), .D(d3), .QN(o3));
  DFFHQNx1_ASAP7_75t_L ff4 (.CLK(clk1), .D(d4), .QN(o4));
endmodule

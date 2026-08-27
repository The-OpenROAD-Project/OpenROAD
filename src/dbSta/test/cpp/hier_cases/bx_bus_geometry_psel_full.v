// TOP: top
// TECH: nangate45
// TARGETS: part_select_port, full_range, depth_1
// CLUE: Part-selects covering the ENTIRE bus on both input and output port
// connections (.a(x[3:0]), .y(z[3:0])); semantically whole-bus but a distinct
// reader path from a plain identifier connection.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1   g0 (.A(a[0]), .ZN(y[0]));
  BUF_X1   g1 (.A(a[1]), .Z(y[1]));
  NAND2_X1 g2 (.A1(a[2]), .A2(a[0]), .ZN(y[2]));
  NOR2_X1  g3 (.A1(a[3]), .A2(a[1]), .ZN(y[3]));
endmodule
module top (x, z);
  input [3:0] x;
  output [3:0] z;
  sub s (.a(x[3:0]), .y(z[3:0]));
endmodule

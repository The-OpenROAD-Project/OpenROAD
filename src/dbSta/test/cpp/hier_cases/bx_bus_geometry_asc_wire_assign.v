// TOP: top
// TECH: nangate45
// TARGETS: asc_range, internal_wire, assign_whole_bus, depth_1
// CLUE: Internal wire declared ASCENDING [0:3], filled by a whole-bus assign
// from a descending [3:0] port, then whole-bus connected to a descending child
// port. Positional reversal happens twice and must cancel exactly.
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
  wire [0:3] w;
  assign w = x;
  sub s (.a(w), .y(z));
endmodule

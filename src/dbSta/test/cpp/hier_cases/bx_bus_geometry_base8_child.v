// TOP: top
// TECH: nangate45
// TARGETS: nonzero_base, whole_bus_connect, depth_1
// CLUE: Child ports [11:8] connected whole-bus to parent [3:0] nets; larger base offset.
module sub (a, y);
  input [11:8] a;
  output [11:8] y;
  INV_X1 g0 (.A(a[11]), .ZN(y[11]));
  INV_X1 g1 (.A(a[10]), .ZN(y[10]));
  INV_X1 g2 (.A(a[9]), .ZN(y[9]));
  INV_X1 g3 (.A(a[8]), .ZN(y[8]));
endmodule
module top (x, z);
  input [3:0] x;
  output [3:0] z;
  sub s (.a(x), .y(z));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: nonzero_base, whole_bus_connect, depth_1
// CLUE: Child ports [7:4] connected whole-bus to parent [3:0] nets; base offset must map positionally (x[3]->a[7]).
module sub (a, y);
  input [7:4] a;
  output [7:4] y;
  INV_X1 g0 (.A(a[7]), .ZN(y[7]));
  INV_X1 g1 (.A(a[6]), .ZN(y[6]));
  INV_X1 g2 (.A(a[5]), .ZN(y[5]));
  INV_X1 g3 (.A(a[4]), .ZN(y[4]));
endmodule
module top (x, z);
  input [3:0] x;
  output [3:0] z;
  sub s (.a(x), .y(z));
endmodule

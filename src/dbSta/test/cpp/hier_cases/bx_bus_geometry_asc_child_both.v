// TOP: top
// TECH: nangate45
// TARGETS: asc_range, whole_bus_connect, depth_1
// CLUE: Child ports ascending [0:3], parent nets descending [3:0], whole-bus connect both directions. Positional mapping x[3]->a[0]; a reader that maps by index instead of position reverses the bus.
module sub (a, y);
  input [0:3] a;
  output [0:3] y;
  INV_X1 g0 (.A(a[0]), .ZN(y[0]));
  INV_X1 g1 (.A(a[1]), .ZN(y[1]));
  INV_X1 g2 (.A(a[2]), .ZN(y[2]));
  INV_X1 g3 (.A(a[3]), .ZN(y[3]));
endmodule
module top (x, z);
  input [3:0] x;
  output [3:0] z;
  wire [3:0] w;
  sub s (.a(x), .y(w));
  BUF_X1 b0 (.A(w[3]), .Z(z[3]));
  BUF_X1 b1 (.A(w[2]), .Z(z[2]));
  BUF_X1 b2 (.A(w[1]), .Z(z[1]));
  BUF_X1 b3 (.A(w[0]), .Z(z[0]));
endmodule

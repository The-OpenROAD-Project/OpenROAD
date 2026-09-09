// TOP: top
// TECH: nangate45
// TARGETS: width_17, whole_bus_connect, depth_1
// CLUE: Width-17 bus through one child with per-bit inverters; probes wide-bus round trip and bus-shape preservation.
module sub (a, y);
  input [16:0] a;
  output [16:0] y;
  INV_X1 g0 (.A(a[16]), .ZN(y[16]));
  INV_X1 g1 (.A(a[15]), .ZN(y[15]));
  INV_X1 g2 (.A(a[14]), .ZN(y[14]));
  INV_X1 g3 (.A(a[13]), .ZN(y[13]));
  INV_X1 g4 (.A(a[12]), .ZN(y[12]));
  INV_X1 g5 (.A(a[11]), .ZN(y[11]));
  INV_X1 g6 (.A(a[10]), .ZN(y[10]));
  INV_X1 g7 (.A(a[9]), .ZN(y[9]));
  INV_X1 g8 (.A(a[8]), .ZN(y[8]));
  INV_X1 g9 (.A(a[7]), .ZN(y[7]));
  INV_X1 g10 (.A(a[6]), .ZN(y[6]));
  INV_X1 g11 (.A(a[5]), .ZN(y[5]));
  INV_X1 g12 (.A(a[4]), .ZN(y[4]));
  INV_X1 g13 (.A(a[3]), .ZN(y[3]));
  INV_X1 g14 (.A(a[2]), .ZN(y[2]));
  INV_X1 g15 (.A(a[1]), .ZN(y[1]));
  INV_X1 g16 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z);
  input [16:0] x;
  output [16:0] z;
  sub s (.a(x), .y(z));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: width_2, whole_bus_connect, depth_1
// CLUE: Width-2 bus through one child with per-bit inverters; probes wide-bus round trip and bus-shape preservation.
module sub (a, y);
  input [1:0] a;
  output [1:0] y;
  INV_X1 g0 (.A(a[1]), .ZN(y[1]));
  INV_X1 g1 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z);
  input [1:0] x;
  output [1:0] z;
  sub s (.a(x), .y(z));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: bus_range_00, top_ports, depth_1
// CLUE: [0:0] one-bit bus only on TOP ports, child is scalar; probes top-port bus-shape preservation for width 1.
module sub (a, y);
  input a;
  output y;
  INV_X1 g0 (.A(a), .ZN(y));
endmodule
module top (x, z);
  input [0:0] x;
  output [0:0] z;
  sub s (.a(x[0]), .y(z[0]));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: chain_depth_6, bus_passthrough, gated_leaf
// CLUE: 6-level wrapper chain carrying a 4-bit bus through every boundary;
// stresses bus port preservation across many nested hierarchy levels.

module leafb (input [3:0] a, output [3:0] z);
  INV_X1 g0 (.A(a[0]), .ZN(z[0]));
  INV_X1 g1 (.A(a[1]), .ZN(z[1]));
  INV_X1 g2 (.A(a[2]), .ZN(z[2]));
  INV_X1 g3 (.A(a[3]), .ZN(z[3]));
endmodule

module b5 (input [3:0] a, output [3:0] z);
  leafb u (.a(a), .z(z));
endmodule

module b4 (input [3:0] a, output [3:0] z);
  b5 u (.a(a), .z(z));
endmodule

module b3 (input [3:0] a, output [3:0] z);
  b4 u (.a(a), .z(z));
endmodule

module b2 (input [3:0] a, output [3:0] z);
  b3 u (.a(a), .z(z));
endmodule

module b1 (input [3:0] a, output [3:0] z);
  b2 u (.a(a), .z(z));
endmodule

module top (input [3:0] a, output [3:0] z);
  b1 u (.a(a), .z(z));
endmodule

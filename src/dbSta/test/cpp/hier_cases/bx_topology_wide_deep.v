// TOP: top
// TECH: nangate45
// TARGETS: depth_5, fanout_4_at_two_levels, wide_and_deep
// CLUE: depth-5 hierarchy with fanout 4 at level 1 (top has 4x w1) and at
// level 3 (w2 has 4x w3); 16 leaves, combining replication and depth in one
// tree.

module w4 (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module w3 (input a, output z);
  w4 u (.a(a), .z(z));
endmodule

module w2 (input [3:0] a, output [3:0] z);
  w3 u0 (.a(a[0]), .z(z[0]));
  w3 u1 (.a(a[1]), .z(z[1]));
  w3 u2 (.a(a[2]), .z(z[2]));
  w3 u3 (.a(a[3]), .z(z[3]));
endmodule

module w1 (input [3:0] a, output [3:0] z);
  w2 u (.a(a), .z(z));
endmodule

module top (input [15:0] a, output [15:0] z);
  w1 u0 (.a(a[3:0]),   .z(z[3:0]));
  w1 u1 (.a(a[7:4]),   .z(z[7:4]));
  w1 u2 (.a(a[11:8]),  .z(z[11:8]));
  w1 u3 (.a(a[15:12]), .z(z[15:12]));
endmodule

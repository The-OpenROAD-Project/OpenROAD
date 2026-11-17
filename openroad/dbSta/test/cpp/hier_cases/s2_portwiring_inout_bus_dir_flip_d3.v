// TARGETS: inout, bus, range_direction_mismatch, depth_3
// CLUE: An INOUT bus is carried through three levels; the middle level flips
// CLUE: the declared range from [0:1] to [1:0], so the positional whole-bus
// CLUE: connection swaps the two inout bits exactly once on the way down.
// CLUE: Both bits are read at the bottom by different gates, so the swap is
// CLUE: visible at two separate top outputs.

module top (a, io, y);
 input [1:0] a;
 inout [0:1] io;
 output [1:0] y;
 l1 u (.p(a), .t(io), .z(y));
endmodule

module l1 (p, t, z);
 input [1:0] p;
 inout [0:1] t;
 output [1:0] z;
 l2 u (.p(p), .t(t), .z(z));
endmodule

module l2 (p, t, z);
 input [1:0] p;
 inout [1:0] t;
 output [1:0] z;
 l3 u (.p(p), .t(t), .z(z));
endmodule

module l3 (p, t, z);
 input [1:0] p;
 inout [1:0] t;
 output [1:0] z;
 XOR2_X1 g0 (.A(p[0]), .B(t[0]), .Z(z[0]));
 AND2_X1 g1 (.A1(p[1]), .A2(t[1]), .ZN(z[1]));
endmodule

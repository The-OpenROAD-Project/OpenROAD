// TOP: top
// TECH: nangate45
// TARGETS: perm_cancel, depth_1, bus, series
// CLUE: Two sibling permuting children in series whose composition is the
// CLUE: identity. The writer must not 'simplify' either instance away, and
// CLUE: must keep both bit permutations exactly.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 wire [3:0] m;
 p1 u1 (.i(a), .o(m));
 p2 u2 (.i(m), .o(y));
endmodule

module p1 (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[3]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[1]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule

module p2 (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[3]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[1]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule

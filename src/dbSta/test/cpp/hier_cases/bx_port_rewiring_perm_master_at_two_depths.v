// TOP: top
// TECH: nangate45
// TARGETS: same_master, mixed_depth, perm_rot1
// CLUE: One permuting master instantiated at depth 1 and again at depth 2 in
// CLUE: the same design: flattening has to give the two copies different
// CLUE: path prefixes while keeping identical bit permutations.

module top (a, y, z);
 input [3:0] a;
 output [3:0] y;
 output [3:0] z;
 p1 u1 (.i(a), .o(y));
 mid u2 (.i(a), .o(z));
endmodule

module mid (i, o);
 input [3:0] i;
 output [3:0] o;
 p1 u (.i(i), .o(o));
endmodule

module p1 (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[1]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[3]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule

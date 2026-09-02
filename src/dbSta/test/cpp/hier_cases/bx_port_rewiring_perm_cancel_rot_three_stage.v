// TOP: top
// TECH: nangate45
// TARGETS: perm_cancel, depth_3, concat_port_conn, modular_arithmetic
// CLUE: rot1 by input concat at level 1, rot1 by input concat at level 2, and
// CLUE: rot2 in the leaf body: the three rotations sum to 4 == identity.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 l1 u (.i(a), .o(y));
endmodule

module l1 (i, o);
 input [3:0] i;
 output [3:0] o;
 l2 u (.i({i[0],i[3],i[2],i[1]}), .o(o));
endmodule

module l2 (i, o);
 input [3:0] i;
 output [3:0] o;
 l3 u (.i({i[0],i[3],i[2],i[1]}), .o(o));
endmodule

module l3 (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[2]), .Z(o[0]));
 BUF_X1 b1 (.A(i[3]), .Z(o[1]));
 BUF_X1 b2 (.A(i[0]), .Z(o[2]));
 BUF_X1 b3 (.A(i[1]), .Z(o[3]));
endmodule

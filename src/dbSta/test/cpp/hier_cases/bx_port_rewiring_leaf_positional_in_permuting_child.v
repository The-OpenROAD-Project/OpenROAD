// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, leaf_cell, perm_rot1, depth_1
// CLUE: Positional leaf-cell connections INSIDE a child that also performs a
// CLUE: 4-bit rotate through its port bits: two orderings interact.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 pleaf u (.i(a), .o(y));
endmodule

module pleaf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (i[1], o[0]);
 BUF_X1 b1 (i[2], o[1]);
 BUF_X1 b2 (i[3], o[2]);
 BUF_X1 b3 (i[0], o[3]);
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: perm_rot1, depth_2, bus, child_body_perm
// CLUE: 4-bit rot1 permutation realised by the wiring between the leaf's
// CLUE: input and output port bits, 2 level(s) below top.
// CLUE: A writer that assumes port bit j maps to port bit j scrambles it.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 m1 u0 (.i(a), .o(y));
endmodule

module m1 (i, o);
 input [3:0] i;
 output [3:0] o;
 pleaf u (.i(i), .o(o));
endmodule

module pleaf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[1]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[3]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule

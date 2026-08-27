// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, depth_3, bus
// CLUE: Every level of a three-deep chain is instantiated positionally, and
// CLUE: the leaf permutes: a positional binding error at any level shows up
// CLUE: as scrambled output bits.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 m1 u (a, y);
endmodule

module m1 (i, o);
 input [3:0] i;
 output [3:0] o;
 m2 u (i, o);
endmodule

module m2 (i, o);
 input [3:0] i;
 output [3:0] o;
 pleaf u (i, o);
endmodule

module pleaf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[1]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[3]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule

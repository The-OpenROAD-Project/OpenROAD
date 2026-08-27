// TOP: top
// TECH: nangate45
// TARGETS: nonansi_header_order, bus, named_conn, depth_2
// CLUE: Header order scrambled at two levels (o before i at level 1, i before
// CLUE: o at level 2) with named bindings and a permuting leaf.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 l1 u (.i(a), .o(y));
endmodule

module l1 (o, i);
 input [3:0] i;
 output [3:0] o;
 l2 u (.i(i), .o(o));
endmodule

module l2 (i, o);
 output [3:0] o;
 input [3:0] i;
 pleaf u (.i(i), .o(o));
endmodule

module pleaf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[3]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[1]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule

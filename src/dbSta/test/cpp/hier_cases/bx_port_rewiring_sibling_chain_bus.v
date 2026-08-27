// TOP: top
// TECH: nangate45
// TARGETS: sibling_chain, bus, no_parent_logic
// CLUE: Bus version of the sibling chain: a 4-bit bus is the only thing the
// CLUE: parent owns, and both children permute it.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 wire [3:0] m;
 pa u1 (.i(a), .o(m));
 pb u2 (.i(m), .o(y));
endmodule

module pa (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[1]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[3]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule

module pb (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[1]), .Z(o[0]));
 BUF_X1 b1 (.A(i[0]), .Z(o[1]));
 BUF_X1 b2 (.A(i[3]), .Z(o[2]));
 BUF_X1 b3 (.A(i[2]), .Z(o[3]));
endmodule

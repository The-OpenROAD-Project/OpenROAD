// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, perm_rev, header_concat
// CLUE: The 4-bit reversal lives in the module HEADER: external port o bit 3 is
// CLUE: internal o[0]. Nothing in the body or the instantiation shows it.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (.i(a), .o(y));
endmodule

module sub (.i(i), .o({o[0],o[1],o[2],o[3]}));
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule

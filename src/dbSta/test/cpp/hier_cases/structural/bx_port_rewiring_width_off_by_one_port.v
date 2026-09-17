// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, off_by_one
// CLUE: Width mismatch of exactly one bit (4-bit net into a 3-bit port) to see
// CLUE: whether the reader drops the whole connection or just the extra bit.

module top (a, y);
 input [3:0] a;
 output [2:0] y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 input [2:0] i;
 output [2:0] o;
 INV_X1 b0 (.A(i[0]), .ZN(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 INV_X1 b2 (.A(i[2]), .ZN(o[2]));
endmodule

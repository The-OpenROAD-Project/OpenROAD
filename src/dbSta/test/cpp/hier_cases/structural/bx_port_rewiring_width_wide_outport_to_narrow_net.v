// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, port_conn, output_truncate
// CLUE: A 4-bit child OUTPUT port bound to a 2-bit top output: the upper two
// CLUE: port bits are driven inside the child but truncated at the boundary.

module top (a, y);
 input [3:0] a;
 output [1:0] y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 input [3:0] i;
 output [3:0] o;
 INV_X1 b0 (.A(i[0]), .ZN(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 INV_X1 b2 (.A(i[2]), .ZN(o[2]));
 INV_X1 b3 (.A(i[3]), .ZN(o[3]));
endmodule

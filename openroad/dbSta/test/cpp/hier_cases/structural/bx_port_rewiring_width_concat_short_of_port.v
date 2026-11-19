// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, concat_port_conn, short_concat
// CLUE: A 3-element concat feeds a 4-bit child input port: the concat is one
// CLUE: bit short, so port bit 3 is unfilled.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (.i({a[1],a[2],a[3]}), .o(y));
endmodule

module sub (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule

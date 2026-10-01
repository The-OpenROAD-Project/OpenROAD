// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, nonansi_header_order, bus, decl_order_mismatch
// CLUE: Bus version: header (o, i) with declarations listed input-first.
// CLUE: Positional binding must use header order for the 4-bit buses too.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (y, a);
endmodule

module sub (o, i);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[1]), .Z(o[0]));
 BUF_X1 b1 (.A(i[0]), .Z(o[1]));
 BUF_X1 b2 (.A(i[3]), .Z(o[2]));
 BUF_X1 b3 (.A(i[2]), .Z(o[3]));
endmodule

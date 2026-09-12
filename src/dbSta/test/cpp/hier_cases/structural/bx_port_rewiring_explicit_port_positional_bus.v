// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, bus, positional_conn, segv_bracket
// CLUE: Bracket: explicit named header ports carrying 4-bit buses, connected
// CLUE: positionally, with a rotate inside the child.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (a, y);
endmodule

module sub (.pi(i), .po(o));
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[1]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[3]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule

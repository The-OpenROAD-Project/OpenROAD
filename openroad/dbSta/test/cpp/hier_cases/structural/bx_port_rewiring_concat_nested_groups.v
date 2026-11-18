// TOP: top
// TECH: nangate45
// TARGETS: concat_port_conn, nested_concat, perm_swap
// CLUE: The port expression is a nested concatenation of two 2-bit groups,
// CLUE: {{a[1],a[0]},{a[3],a[2]}}, which swaps the halves and reverses inside
// CLUE: each half.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 pbuf u (.i({{a[1],a[0]},{a[3],a[2]}}), .o(y));
endmodule

module pbuf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule

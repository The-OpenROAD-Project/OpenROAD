// TOP: top
// TECH: nangate45
// TARGETS: concat_port_conn, perm_rev, depth_1
// CLUE: Reversal expressed purely in the port connection: .i({a[0],a[1],
// CLUE: a[2],a[3]}) into a straight-through child.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 pbuf u (.i({a[0],a[1],a[2],a[3]}), .o(y));
endmodule

module pbuf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule

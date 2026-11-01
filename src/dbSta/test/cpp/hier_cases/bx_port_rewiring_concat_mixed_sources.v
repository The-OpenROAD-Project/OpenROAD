// TOP: top
// TECH: nangate45
// TARGETS: concat_port_conn, two_source_buses
// CLUE: A child input port fed by a concat that interleaves bits from two
// CLUE: different top buses in scrambled index order.

module top (a, b, y);
 input [3:0] a;
 input [3:0] b;
 output [3:0] y;
 pbuf u (.i({a[3],b[0],a[1],b[2]}), .o(y));
endmodule

module pbuf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule

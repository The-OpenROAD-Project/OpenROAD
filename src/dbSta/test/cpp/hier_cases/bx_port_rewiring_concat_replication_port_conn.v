// TOP: top
// TECH: nangate45
// TARGETS: concat_port_conn, replication, probe
// CLUE: PROBE: a replication expression {2{a[1:0]}} as the port connection.
// CLUE: Legal Verilog-2005 concatenation form, rare in netlists.

module top (a, y);
 input [1:0] a;
 output [3:0] y;
 pbuf u (.i({2{a[1:0]}}), .o(y));
endmodule

module pbuf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 INV_X1 b2 (.A(i[2]), .ZN(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule

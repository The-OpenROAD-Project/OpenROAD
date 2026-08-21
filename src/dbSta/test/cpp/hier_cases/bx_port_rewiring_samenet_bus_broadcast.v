// TOP: top
// TECH: nangate45
// TARGETS: shared_net, concat_port_conn, broadcast
// CLUE: One scalar bit replicated into all four bits of a child bus port via
// CLUE: an explicit concat (no replication operator).

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 pbuf u (.i({a[0],a[0],a[0],a[0]}), .o(y));
endmodule

module pbuf (i, o);
 input [3:0] i;
 output [3:0] o;
 INV_X1 b0 (.A(i[0]), .ZN(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 INV_X1 b2 (.A(i[2]), .ZN(o[2]));
 INV_X1 b3 (.A(i[3]), .ZN(o[3]));
endmodule

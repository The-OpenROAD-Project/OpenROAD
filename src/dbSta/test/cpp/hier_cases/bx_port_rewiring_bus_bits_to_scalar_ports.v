// TOP: top
// TECH: nangate45
// TARGETS: bit_select_port_conn, bus_to_scalar
// CLUE: A top bus is delivered to four scalar child input ports in scrambled
// CLUE: index order; child is a straight buffer bank.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sbuf u (.p0(a[2]), .p1(a[3]), .p2(a[0]), .p3(a[1]),
         .q0(y[0]), .q1(y[1]), .q2(y[2]), .q3(y[3]));
endmodule

module sbuf (p0, p1, p2, p3, q0, q1, q2, q3);
 input p0, p1, p2, p3;
 output q0, q1, q2, q3;
 BUF_X1 b0 (.A(p0), .Z(q0));
 BUF_X1 b1 (.A(p1), .Z(q1));
 BUF_X1 b2 (.A(p2), .Z(q2));
 BUF_X1 b3 (.A(p3), .Z(q3));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: concat_port_conn, repeated_bit, fanout
// CLUE: A child input port concat repeats parent bits: {a[0],a[0],a[1],a[1]}.
// CLUE: Two port bits share one parent net bit.

module top (a, y);
 input [1:0] a;
 output [3:0] y;
 pbuf u (.i({a[0],a[0],a[1],a[1]}), .o(y));
endmodule

module pbuf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 INV_X1 b3 (.A(i[3]), .ZN(o[3]));
endmodule

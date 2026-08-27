// TOP: top
// TECH: nangate45
// TARGETS: named_conn, empty_port_conn, bus, dangling_in
// CLUE: .i() explicitly leaves a 4-bit child input port unconnected; that port
// CLUE: is unused inside the child so nothing floats logically.

module top (a, y);
 input a;
 output y;
 sub u (.i(), .k(a), .o(y));
endmodule

module sub (i, k, o);
 input [3:0] i;
 input k;
 output o;
 BUF_X1 g (.A(k), .Z(o));
endmodule

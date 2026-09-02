// TARGETS: bus_two_sources, whole_bus_connect, depth_1
// CLUE: One parent bus t is driven from TWO different places -- t[3:2] by a
// CLUE: child instance's output port, t[1:0] by two top-level gates -- and the
// CLUE: whole of t is then handed to a second child's 4-bit input port. The
// CLUE: second child reverses, so each output bit names exactly one source.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 wire [3:0] t;
 h u1 (.i(a[3:2]), .o(t[3:2]));
 BUF_X1 g0 (.A(a[0]), .Z(t[0]));
 INV_X1 g1 (.A(a[1]), .ZN(t[1]));
 h2 u2 (.i(t), .o(y));
endmodule

module h (i, o);
 input [1:0] i;
 output [1:0] o;
 INV_X1 b0 (.A(i[0]), .ZN(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
endmodule

module h2 (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 c0 (.A(i[0]), .Z(o[3]));
 BUF_X1 c1 (.A(i[1]), .Z(o[2]));
 BUF_X1 c2 (.A(i[2]), .Z(o[1]));
 BUF_X1 c3 (.A(i[3]), .Z(o[0]));
endmodule

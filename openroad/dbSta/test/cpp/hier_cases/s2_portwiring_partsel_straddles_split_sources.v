// TARGETS: bus_two_sources, part_select_port_conn, straddle, depth_1
// CLUE: t[3:2] is driven by a child's output port and t[1:0] by top-level
// CLUE: gates, and the part-select t[2:1] handed to a second child STRADDLES
// CLUE: the boundary between the two source regions, so one bit of the port
// CLUE: comes from the hierarchy and the other from a flat gate.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 wire [3:0] t;
 h u1 (.i(a[3:2]), .o(t[3:2]));
 BUF_X1 g0 (.A(a[0]), .Z(t[0]));
 INV_X1 g1 (.A(a[1]), .ZN(t[1]));
 h u2 (.i(t[2:1]), .o(y[1:0]));
 BUF_X1 g2 (.A(t[3]), .Z(y[3]));
 INV_X1 g3 (.A(t[0]), .ZN(y[2]));
endmodule

module h (i, o);
 input [1:0] i;
 output [1:0] o;
 INV_X1 b0 (.A(i[0]), .ZN(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
endmodule

// TARGETS: asc_range, concat_port_conn, top_level, depth_1
// CLUE: Both the TOP ports and the child ports are ASCENDING, and the binding
// CLUE: is an explicit concat that reverses. The declared range and the concat
// CLUE: order pull in opposite directions, so a writer that normalises either
// CLUE: one to msb-first without adjusting the other flips every bit.

module top (x, z);
 input [0:3] x;
 output [0:3] z;
 sub s (.a({x[3],x[2],x[1],x[0]}), .y({z[3],z[2],z[1],z[0]}));
endmodule

module sub (a, y);
 input [0:3] a;
 output [0:3] y;
 BUF_X1 g0 (.A(a[0]), .Z(y[0]));
 INV_X1 g1 (.A(a[1]), .ZN(y[1]));
 BUF_X1 g2 (.A(a[2]), .Z(y[2]));
 INV_X1 g3 (.A(a[3]), .ZN(y[3]));
endmodule

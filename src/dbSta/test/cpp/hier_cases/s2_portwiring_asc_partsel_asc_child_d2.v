// TARGETS: asc_range, part_select_port_conn, asc_part_select, depth_2
// CLUE: Every bus in the design is ASCENDING, including the part-select
// CLUE: a[1:2] handed to a [0:1] grandchild port. An ascending part-select of
// CLUE: an ascending port is the shape that breaks if the reader or writer
// CLUE: normalises ranges to msb-first anywhere along the path.

module top (x, z);
 input [0:3] x;
 output [0:3] z;
 mid u (.a(x), .y(z));
endmodule

module mid (a, y);
 input [0:3] a;
 output [0:3] y;
 leaf u (.a(a[1:2]), .y(y[1:2]));
 BUF_X1 p0 (.A(a[0]), .Z(y[0]));
 INV_X1 p3 (.A(a[3]), .ZN(y[3]));
endmodule

module leaf (a, y);
 input [0:1] a;
 output [0:1] y;
 INV_X1 g0 (.A(a[0]), .ZN(y[0]));
 BUF_X1 g1 (.A(a[1]), .Z(y[1]));
endmodule

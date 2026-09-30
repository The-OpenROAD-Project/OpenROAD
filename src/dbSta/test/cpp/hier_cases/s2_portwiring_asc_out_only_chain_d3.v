// TARGETS: asc_range, output_side_only, whole_bus_connect, depth_3
// CLUE: Only the OUTPUT buses are ascending, and they are ascending at all
// CLUE: three inner levels, so the single reversal happens at the top boundary
// CLUE: where a [0:3] child port meets a [3:0] top port. Input side stays
// CLUE: descending throughout, which isolates the output-side direction path.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 l1 u (.i(a), .o(y));
endmodule

module l1 (i, o);
 input [3:0] i;
 output [0:3] o;
 l2 u (.i(i), .o(o));
endmodule

module l2 (i, o);
 input [3:0] i;
 output [0:3] o;
 l3 u (.i(i), .o(o));
endmodule

module l3 (i, o);
 input [3:0] i;
 output [0:3] o;
 BUF_X1 g0 (.A(i[0]), .Z(o[0]));
 INV_X1 g1 (.A(i[1]), .ZN(o[1]));
 BUF_X1 g2 (.A(i[2]), .Z(o[2]));
 INV_X1 g3 (.A(i[3]), .ZN(o[3]));
endmodule

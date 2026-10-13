// TARGETS: concat_port_conn, whole_bus_in_concat, part_select_output_port, depth_2
// CLUE: A whole 3-bit bus port and one bit of a 2-bit bus port are concatenated
// CLUE: into a 4-bit child input, and the child's result comes back through a
// CLUE: part-select of a 5-bit output port. Uneven bus widths on both sides.

module top (a, b, y);
 input [1:0] a;
 input [2:0] b;
 output [4:0] y;
 mid u (.pa(a), .pb(b), .o(y));
endmodule

module mid (pa, pb, o);
 input [1:0] pa;
 input [2:0] pb;
 output [4:0] o;
 leaf u (.i({pb, pa[0]}), .o(o[3:0]));
 INV_X1 g (.A(pa[1]), .ZN(o[4]));
endmodule

module leaf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 INV_X1 b3 (.A(i[3]), .ZN(o[3]));
endmodule

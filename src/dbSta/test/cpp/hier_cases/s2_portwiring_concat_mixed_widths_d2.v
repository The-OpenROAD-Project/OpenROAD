// TARGETS: concat_port_conn, mixed_element_widths, depth_2
// CLUE: The concat feeding the leaf's 4-bit port has elements of three
// CLUE: different shapes -- a 2-bit part-select, a scalar port and a single
// CLUE: bit-select -- so the reader has to walk the concat by BITS, not by
// CLUE: elements, to land each source on the right port bit.

module top (a, b, y);
 input [3:0] a;
 input b;
 output [3:0] y;
 mid u (.pa(a), .pb(b), .o(y));
endmodule

module mid (pa, pb, o);
 input [3:0] pa;
 input pb;
 output [3:0] o;
 leaf u (.i({pa[1:0], pb, pa[3]}), .o(o));
endmodule

module leaf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 g0 (.A(i[0]), .Z(o[0]));
 INV_X1 g1 (.A(i[1]), .ZN(o[1]));
 BUF_X1 g2 (.A(i[2]), .Z(o[2]));
 INV_X1 g3 (.A(i[3]), .ZN(o[3]));
endmodule

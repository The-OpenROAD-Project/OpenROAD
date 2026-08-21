// TARGETS: concat_port_conn, three_sources, odd_width, depth_2
// CLUE: A 5-bit child port is fed by a concat that interleaves bits of three
// CLUE: different parent ports in a non-monotonic order. Odd width keeps the
// CLUE: bit map from being a symmetric swap, and every output bit is a
// CLUE: different function (BUF/INV alternating) of a different source bit.

module top (a, b, c, y);
 input [1:0] a;
 input [1:0] b;
 input c;
 output [4:0] y;
 mid u (.pa(a), .pb(b), .pc(c), .o(y));
endmodule

module mid (pa, pb, pc, o);
 input [1:0] pa;
 input [1:0] pb;
 input pc;
 output [4:0] o;
 leaf u (.i({pb[0], pa[1], pc, pa[0], pb[1]}), .o(o));
endmodule

module leaf (i, o);
 input [4:0] i;
 output [4:0] o;
 BUF_X1 g0 (.A(i[0]), .Z(o[0]));
 INV_X1 g1 (.A(i[1]), .ZN(o[1]));
 BUF_X1 g2 (.A(i[2]), .Z(o[2]));
 INV_X1 g3 (.A(i[3]), .ZN(o[3]));
 BUF_X1 g4 (.A(i[4]), .Z(o[4]));
endmodule

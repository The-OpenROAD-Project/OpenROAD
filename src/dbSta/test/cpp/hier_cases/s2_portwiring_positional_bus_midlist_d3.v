// TARGETS: positional_conn, bus_in_port_list, concat_positional_arg, depth_3
// CLUE: Every level is wired POSITIONALLY and the bus sits in the middle of a
// CLUE: six-port list flanked by scalars, so a port-order slip moves a scalar
// CLUE: into a bus slot. At the deepest boundary the bus argument is a concat
// CLUE: that reverses it, which is the only permutation in the design.

module top (a, b, c, y, z, w);
 input a;
 input [3:0] b;
 input c;
 output y;
 output [3:0] z;
 output w;
 l1 u (a, b, c, y, z, w);
endmodule

module l1 (p, q, r, s, t, v);
 input p;
 input [3:0] q;
 input r;
 output s;
 output [3:0] t;
 output v;
 l2 u (p, q, r, s, t, v);
endmodule

module l2 (p, q, r, s, t, v);
 input p;
 input [3:0] q;
 input r;
 output s;
 output [3:0] t;
 output v;
 l3 u (p, {q[0],q[1],q[2],q[3]}, r, s, t, v);
endmodule

module l3 (p, q, r, s, t, v);
 input p;
 input [3:0] q;
 input r;
 output s;
 output [3:0] t;
 output v;
 BUF_X1 g0 (.A(p), .Z(s));
 INV_X1 g1 (.A(r), .ZN(v));
 BUF_X1 g2 (.A(q[0]), .Z(t[0]));
 INV_X1 g3 (.A(q[1]), .ZN(t[1]));
 BUF_X1 g4 (.A(q[2]), .Z(t[2]));
 INV_X1 g5 (.A(q[3]), .ZN(t[3]));
endmodule

// TARGETS: asc_range, range_direction_mismatch, mixed_directions_one_module, depth_2
// CLUE: One module owns an ASCENDING and a DESCENDING bus port of the same
// CLUE: width in each direction, and the parent crosses them: the ascending
// CLUE: input feeds the descending output and vice versa. Both range
// CLUE: conventions live in the same port list of the same module.

module top (p, q, r, s);
 input [0:3] p;
 input [3:0] q;
 output [0:3] r;
 output [3:0] s;
 mid u (.a(p), .b(q), .y(r), .z(s));
endmodule

module mid (a, b, y, z);
 input [0:3] a;
 input [3:0] b;
 output [0:3] y;
 output [3:0] z;
 leaf u (.i(a), .j(b), .o(y), .k(z));
endmodule

module leaf (i, j, o, k);
 input [0:3] i;
 input [3:0] j;
 output [0:3] o;
 output [3:0] k;
 BUF_X1 g0 (.A(j[0]), .Z(o[0]));
 INV_X1 g1 (.A(j[1]), .ZN(o[1]));
 BUF_X1 g2 (.A(j[2]), .Z(o[2]));
 INV_X1 g3 (.A(j[3]), .ZN(o[3]));
 INV_X1 h0 (.A(i[0]), .ZN(k[0]));
 BUF_X1 h1 (.A(i[1]), .Z(k[1]));
 INV_X1 h2 (.A(i[2]), .ZN(k[2]));
 BUF_X1 h3 (.A(i[3]), .Z(k[3]));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: swap_symmetric, two_instances, scalar_ports
// CLUE: Two instances of the same asymmetric child, wired with swapped
// CLUE: arguments: u1 gets (a,b), u2 gets (b,a). Any port confusion makes
// CLUE: the two output cones identical instead of complementary.

module top (a, b, y0, y1);
 input a, b;
 output y0, y1;
 sub u1 (.p(a), .q(b), .z(y0));
 sub u2 (.p(b), .q(a), .z(y1));
endmodule

module sub (p, q, z);
 input p, q;
 output z;
 wire qb;
 INV_X1 n (.A(q), .ZN(qb));
 AND2_X1 g (.A1(p), .A2(qb), .ZN(z));
endmodule

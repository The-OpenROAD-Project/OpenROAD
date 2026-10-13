// TOP: top
// TECH: nangate45
// TARGETS: swap_symmetric, depth_2, two_instances
// CLUE: Swap-symmetric instantiation one level down, so flattening has to
// CLUE: keep two differently-wired copies of the same grandchild.

module top (a, b, y0, y1);
 input a, b;
 output y0, y1;
 mid u (.p(a), .q(b), .z0(y0), .z1(y1));
endmodule

module mid (p, q, z0, z1);
 input p, q;
 output z0, z1;
 sub u1 (.p(p), .q(q), .z(z0));
 sub u2 (.p(q), .q(p), .z(z1));
endmodule

module sub (p, q, z);
 input p, q;
 output z;
 wire qb;
 INV_X1 n (.A(q), .ZN(qb));
 AND2_X1 g (.A1(p), .A2(qb), .ZN(z));
endmodule

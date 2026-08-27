// TOP: top
// TECH: nangate45
// TARGETS: swap_symmetric, two_instances, positional_conn
// CLUE: Same swap-symmetric pair, expressed positionally.

module top (a, b, y0, y1);
 input a, b;
 output y0, y1;
 sub u1 (a, b, y0);
 sub u2 (b, a, y1);
endmodule

module sub (p, q, z);
 input p, q;
 output z;
 wire qb;
 INV_X1 n (.A(q), .ZN(qb));
 AND2_X1 g (.A1(p), .A2(qb), .ZN(z));
endmodule

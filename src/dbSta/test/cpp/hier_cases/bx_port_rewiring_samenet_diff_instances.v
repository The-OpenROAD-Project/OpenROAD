// TOP: top
// TECH: nangate45
// TARGETS: shared_net, cross_instance
// CLUE: One top net feeds input ports of two different child instances, and
// CLUE: both ports of the second one.

module top (a, b, y0, y1);
 input a, b;
 output y0, y1;
 sub u1 (.p(a), .q(b), .z(y0));
 sub u2 (.p(a), .q(a), .z(y1));
endmodule

module sub (p, q, z);
 input p, q;
 output z;
 wire qb;
 INV_X1 n (.A(q), .ZN(qb));
 OR2_X1 g (.A1(p), .A2(qb), .ZN(z));
endmodule

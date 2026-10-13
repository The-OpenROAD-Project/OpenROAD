// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, named_conn, two_instances
// CLUE: Two instances of the same master: one bound positionally, one bound
// CLUE: by name with the .p/.q/.z items listed in reverse textual order.

module top (a, b, y0, y1);
 input a, b;
 output y0, y1;
 sub u1 (a, b, y0);
 sub u2 (.z(y1), .q(b), .p(a));
endmodule

module sub (p, q, z);
 input p, q;
 output z;
 wire qb;
 INV_X1 n (.A(q), .ZN(qb));
 AND2_X1 g (.A1(p), .A2(qb), .ZN(z));
endmodule

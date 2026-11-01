// TOP: top
// TECH: nangate45
// TARGETS: same_master, per_instance_connectivity, dangling_in
// CLUE: Two instances of one master: u1 connects every port, u2 leaves the q
// CLUE: input and the w output unconnected. Per-instance port connectivity
// CLUE: must not be shared between instances of the same module.

module top (a, b, y0, y1, y2);
 input a, b;
 output y0, y1, y2;
 sub u1 (.p(a), .q(b), .z(y0), .w(y1));
 sub u2 (.p(a), .q(), .z(y2), .w());
endmodule

module sub (p, q, z, w);
 input p, q;
 output z, w;
 INV_X1 g (.A(p), .ZN(z));
 INV_X1 h (.A(q), .ZN(w));
endmodule

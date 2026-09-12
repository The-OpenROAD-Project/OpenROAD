// TOP: top
// TECH: nangate45
// TARGETS: same_master, per_instance_connectivity, dangling_out
// CLUE: Both instances drive the q input, but only u1 uses the w output; u2's
// CLUE: w cone is live inside the child yet has no parent load.

module top (a, b, y0, y1, y2);
 input a, b;
 output y0, y1, y2;
 sub u1 (.p(a), .q(b), .z(y0), .w(y1));
 sub u2 (.p(b), .q(a), .z(y2), .w());
endmodule

module sub (p, q, z, w);
 input p, q;
 output z, w;
 XOR2_X1 g (.A(p), .B(q), .Z(z));
 XNOR2_X1 h (.A(p), .B(q), .ZN(w));
endmodule

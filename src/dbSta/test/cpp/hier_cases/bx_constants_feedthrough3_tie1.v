// TOP: top
// TECH: nangate45
// TARGETS: const_feedthrough, depth_3, tie1
// CLUE: same 3-level feedthrough as bx_constants_feedthrough3 but with 1'b1
// and direct port-to-port connection (no assign inside) — brackets whether an
// inner assign matters.
module k3 (input p, output q);
  BUF_X1 b (.A(p), .Z(q));
endmodule

module k2 (input p, output q);
  k3 i3 (.p(p), .q(q));
endmodule

module k1 (input p, output q);
  k2 i2 (.p(p), .q(q));
endmodule

module top (input a, output y);
  wire t;
  k1 i1 (.p(1'b1), .q(t));
  AND2_X1 g (.A1(a), .A2(t), .ZN(y));
endmodule

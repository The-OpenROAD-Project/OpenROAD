// TOP: top
// TECH: nangate45
// TARGETS: alias_reconvergence
// CLUE: two alias copies of the same input reconverge at an XOR (constant-0 cone).

module top (input i, input zi, output o, output zo);
  wire p, q;
  assign p = i;
  assign q = i;
  XOR2_X1 g0 (.A(p), .B(q), .Z(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule

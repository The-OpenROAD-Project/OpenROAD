// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, mapping_probe, no_port_nets, dead_cone
// CLUE: positional NAND2_X1 whose three nets are all internal (none is a top
// port), output dead. Distinguishes port-direction rejection from a generic
// positional mis-binding on liberty cells.
module top (input p1, input p2, output y);
  wire n1;
  wire n2;
  wire n3;
  wire dz;
  INV_X1 keep (.A(p1), .ZN(y));
  BUF_X1 b1 (.A(p1), .Z(n1));
  BUF_X1 b2 (.A(p2), .Z(n2));
  NAND2_X1 g2 (n1, n2, n3);
  INV_X1 sink (.A(n3), .ZN(dz));
endmodule

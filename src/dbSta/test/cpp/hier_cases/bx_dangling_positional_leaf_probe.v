// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, mapping_probe, dead_cone
// CLUE: read off the positional->pin mapping for a liberty cell: NAND2_X1 g2
// (p1, p2, z) with z given a real (but dead) load so it cannot be swept.
// Whole NAND cone is dead, so LEC is blind: structural read-out only.
module top (input p1, input p2, output y);
  wire z;
  wire dz;
  INV_X1 keep (.A(p1), .ZN(y));
  NAND2_X1 g2 (p1, p2, z);
  INV_X1 sink (.A(z), .ZN(dz));
endmodule

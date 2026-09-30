// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, four_pin_cell, mapping_probe, dead_cone
// CLUE: four-pin cell positional: MUX2_X1 g (p1, p2, p3, z) with z dead.
// Reads off how many of the four ordered connections survive.
module top (input p1, input p2, input p3, output y);
  wire z;
  wire dz;
  INV_X1 keep (.A(p1), .ZN(y));
  MUX2_X1 g2 (p1, p2, p3, z);
  INV_X1 sink (.A(z), .ZN(dz));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, two_pin_cell, offset_probe, dead_cone
// CLUE: BUF_X1 g2 (p1, z); a two-pin liberty cell has fewer positional args
// than the suspected offset of 2, so the expectation is zero surviving pins.
module top (input p1, output y);
  wire z;
  wire dz;
  INV_X1 keep (.A(p1), .ZN(y));
  BUF_X1 g2 (p1, z);
  INV_X1 sink (.A(z), .ZN(dz));
endmodule

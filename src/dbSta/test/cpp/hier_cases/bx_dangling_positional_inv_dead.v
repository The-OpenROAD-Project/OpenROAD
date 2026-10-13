// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, two_pin_cell, dead_cone
// CLUE: two-pin cell positional: INV_X1 g2 (p1, d); with d dead. Does a 2-pin
// liberty cell bind (A,ZN) in order?
module top (input p1, output y);
  wire d;
  wire dd;
  INV_X1 keep (.A(p1), .ZN(y));
  INV_X1 g2 (p1, d);
  BUF_X1 sink (.A(d), .Z(dd));
endmodule

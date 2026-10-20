// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, two_pin_cell, live_cone
// CLUE: two-pin cell positional feeding a top output: INV_X1 g1 (p1, w);
// makes any 2-pin mis-binding LEC-visible.
module top (input p1, output y);
  wire w;
  INV_X1 g1 (p1, w);
  BUF_X1 o1 (.A(w), .Z(y));
endmodule

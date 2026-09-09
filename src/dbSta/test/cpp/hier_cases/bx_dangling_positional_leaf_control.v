// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional_control
// CLUE: control for the leaf positional hole: full ordered connection list on a
// liberty cell, no empty slot. Isolates empty-slot rejection from
// positional-on-leaf-cell rejection.
module top (input x, input x2, output y);
  wire d;
  INV_X1 g1 (.A(x), .ZN(y));
  NAND2_X1 g2 (x, x2, d);
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, live_cone
// CLUE: same positional NAND2_X1 but its output now reaches a top output, so the
// mis-binding seen in the dead variant must become LEC-visible.
module top (input p1, input p2, output y);
  wire z;
  NAND2_X1 g2 (p1, p2, z);
  BUF_X1 o1 (.A(z), .Z(y));
endmodule

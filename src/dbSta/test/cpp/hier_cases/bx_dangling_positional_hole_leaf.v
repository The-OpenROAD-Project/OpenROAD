// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, positional_empty_slot
// CLUE: empty ordered connection on a LIBRARY cell: NAND2_X1 g2 (x, , d);
// legal Verilog-2005; leaves A2 dangling on a liberty cell.
module top (input x, output y);
  wire d;
  INV_X1 g1 (.A(x), .ZN(y));
  NAND2_X1 g2 (x, , d);
endmodule

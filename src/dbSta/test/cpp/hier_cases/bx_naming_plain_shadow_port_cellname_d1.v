// TOP: top
// TECH: nangate45
// TARGETS: cellname_shadow, port, depth_1
// CLUE: Top-level ports named DFF_X1 (input) and NAND2_X1 (output).
module top (DFF_X1, NAND2_X1);
  input DFF_X1;
  output NAND2_X1;
  wire n;
  INV_X1 u1 (.A(DFF_X1), .ZN(n));
  INV_X1 u2 (.A(n), .ZN(NAND2_X1));
endmodule

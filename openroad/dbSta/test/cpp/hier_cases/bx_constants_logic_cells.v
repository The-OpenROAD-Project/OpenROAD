// TOP: top
// TECH: nangate45
// TARGETS: logic_cells
// CLUE: constants produced by LOGIC0_X1/LOGIC1_X1 tie cells driving nets —
// writer must keep the cells (or provably-equivalent literals); compare
// against bx_constants_literal_same.
module top (input a, output y0, output y1);
  wire c0, c1;
  LOGIC0_X1 l0 (.Z(c0));
  LOGIC1_X1 l1 (.Z(c1));
  OR2_X1 g0 (.A1(a), .A2(c0), .ZN(y0));
  AND2_X1 g1 (.A1(a), .A2(c1), .ZN(y1));
endmodule

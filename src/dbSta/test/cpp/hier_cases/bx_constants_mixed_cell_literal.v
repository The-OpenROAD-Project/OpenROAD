// TOP: top
// TECH: nangate45
// TARGETS: logic_cells, literal_ties, mixed
// CLUE: LOGIC0_X1 cell constant AND a literal 1'b1 tie in the same module —
// mixed constant representations must both survive.
module top (input a, output y0, output y1);
  wire c0;
  LOGIC0_X1 l0 (.Z(c0));
  OR2_X1 g0 (.A1(a), .A2(c0), .ZN(y0));
  AND2_X1 g1 (.A1(a), .A2(1'b1), .ZN(y1));
endmodule

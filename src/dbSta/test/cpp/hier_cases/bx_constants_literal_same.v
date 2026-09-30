// TOP: top
// TECH: nangate45
// TARGETS: literal_ties
// CLUE: the SAME circuit as bx_constants_logic_cells but with literal 1'b0 /
// 1'b1 pin ties instead of LOGIC cells — watch whether writers converge the
// two representations.
module top (input a, output y0, output y1);
  OR2_X1 g0 (.A1(a), .A2(1'b0), .ZN(y0));
  AND2_X1 g1 (.A1(a), .A2(1'b1), .ZN(y1));
endmodule

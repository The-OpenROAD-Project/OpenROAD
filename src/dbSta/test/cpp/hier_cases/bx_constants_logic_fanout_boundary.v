// TOP: top
// TECH: nangate45
// TARGETS: logic_cells, boundary_crossing, const_fanout
// CLUE: one LOGIC1_X1 at top fans out DOWN into two submodule instances via
// their input ports — a tie-cell net crossing two boundaries downward.
module sub (input c, input a, output y);
  AND2_X1 g (.A1(a), .A2(c), .ZN(y));
endmodule

module top (input a, input b, output y1, output y2);
  wire c1;
  LOGIC1_X1 l1 (.Z(c1));
  sub s1 (.c(c1), .a(a), .y(y1));
  sub s2 (.c(c1), .a(b), .y(y2));
endmodule

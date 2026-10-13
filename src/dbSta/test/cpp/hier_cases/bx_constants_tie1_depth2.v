// TOP: top
// TECH: nangate45
// TARGETS: tie1, depth_2
// CLUE: literal 1'b1 tie-off one level down inside a submodule.
module top (a, y);
  input a;
  output y;
  mid m (.a(a), .y(y));
endmodule

module mid (a, y);
  input a;
  output y;
  NAND2_X1 u1 (.A1(a), .A2(1'b1), .ZN(y));
endmodule

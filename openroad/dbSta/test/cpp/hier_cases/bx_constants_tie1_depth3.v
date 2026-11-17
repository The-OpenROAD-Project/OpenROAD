// TOP: top
// TECH: nangate45
// TARGETS: tie1, depth_3
// CLUE: literal 1'b1 tie-off two levels down (top->mid->leaf).
module top (a, y);
  input a;
  output y;
  mid m (.a(a), .y(y));
endmodule

module mid (a, y);
  input a;
  output y;
  leaf l (.a(a), .y(y));
endmodule

module leaf (a, y);
  input a;
  output y;
  NAND2_X1 u1 (.A1(a), .A2(1'b1), .ZN(y));
endmodule

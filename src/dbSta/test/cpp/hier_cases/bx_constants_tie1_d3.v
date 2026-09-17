// TOP: top
// TECH: nangate45
// TARGETS: tie1, depth_3
// CLUE: 1'b1 tie-off buried two hierarchy levels down; constant must survive
// two levels of flattening / hierarchical re-emission.
module leaf (a, y);
  input a;
  output y;
  OR2_X1 g1 (.A1(a), .A2(1'b1), .ZN(y));
endmodule

module mid (a, y);
  input a;
  output y;
  leaf l1 (.a(a), .y(y));
endmodule

module top (a, y);
  input a;
  output y;
  mid m1 (.a(a), .y(y));
endmodule

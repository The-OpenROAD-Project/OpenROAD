// TOP: top
// TECH: nangate45
// TARGETS: module_reuse, depths_1_2_3, three_branches
// CLUE: module rm3 instantiated at depth 1, depth 2, and depth 3 in three
// separate branches of top; single definition, three hierarchy contexts.

module rm3 (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module w2 (input a, output z);
  rm3 u (.a(a), .z(z));
endmodule

module w3b (input a, output z);
  rm3 u (.a(a), .z(z));
endmodule

module w3a (input a, output z);
  w3b u (.a(a), .z(z));
endmodule

module top (input a, input b, input c, output x, output y, output z);
  rm3 d1 (.a(a), .z(x));
  w2 d2 (.a(b), .z(y));
  w3a d3 (.a(c), .z(z));
endmodule

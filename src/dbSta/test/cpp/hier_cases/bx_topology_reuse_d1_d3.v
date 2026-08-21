// TOP: top
// TECH: nangate45
// TARGETS: module_reuse, depth_1_and_depth_3, different_branches
// CLUE: module rm instantiated directly by top (depth 1) AND at depth 3
// inside another branch; definition must be emitted once, instances in both
// contexts preserved.

module rm (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module lvl2 (input a, output z);
  rm u (.a(a), .z(z));
endmodule

module lvl1 (input a, output z);
  lvl2 u (.a(a), .z(z));
endmodule

module top (input a, input b, output x, output y);
  rm d1 (.a(a), .z(x));
  lvl1 d3 (.a(b), .z(y));
endmodule

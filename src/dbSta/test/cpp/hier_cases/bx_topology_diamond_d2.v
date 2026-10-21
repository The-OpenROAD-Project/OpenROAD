// TOP: top
// TECH: nangate45
// TARGETS: diamond_reuse, depth_2_parents, shared_leaf
// CLUE: diamond one level deeper: shared leaf under two different parent
// types which are each wrapped in another module before reaching top.

module dleaf (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module pa (input a, output z);
  dleaf u (.a(a), .z(z));
endmodule

module pb (input a, output z);
  dleaf u (.a(a), .z(z));
endmodule

module wa (input a, output z);
  pa u (.a(a), .z(z));
endmodule

module wb (input a, output z);
  pb u (.a(a), .z(z));
endmodule

module top (input a, input b, output x, output y);
  wa i1 (.a(a), .z(x));
  wb i2 (.a(b), .z(y));
endmodule

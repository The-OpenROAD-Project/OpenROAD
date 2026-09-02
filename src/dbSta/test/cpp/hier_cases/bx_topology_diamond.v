// TOP: top
// TECH: nangate45
// TARGETS: diamond_reuse, shared_leaf_two_parent_types
// CLUE: module dleaf instantiated by two DIFFERENT parent module types, both
// instantiated by top. Hier writer must emit dleaf's definition exactly once
// and not uniquify it needlessly.

module dleaf (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module pa (input a, output z);
  dleaf u (.a(a), .z(z));
endmodule

module pb (input a, output z);
  dleaf u (.a(a), .z(z));
endmodule

module top (input a, input b, output x, output y);
  pa i1 (.a(a), .z(x));
  pb i2 (.a(b), .z(y));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: diamond_reuse, asymmetric_depth, shared_leaf
// CLUE: same leaf module reached at depth 2 via one branch and depth 3 via
// the other; uniquification/definition-emission must handle unequal depths.

module aleaf (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module pshort (input a, output z);
  aleaf u (.a(a), .z(z));
endmodule

module pdeep2 (input a, output z);
  aleaf u (.a(a), .z(z));
endmodule

module pdeep1 (input a, output z);
  pdeep2 u (.a(a), .z(z));
endmodule

module top (input a, input b, output x, output y);
  pshort i1 (.a(a), .z(x));
  pdeep1 i2 (.a(b), .z(y));
endmodule

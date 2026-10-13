// TOP: top
// TECH: nangate45
// TARGETS: escaped_module, char_plus, depth_2
// CLUE: module named \m+m  instantiated normally; hier writer must
// re-emit the escaped module name in the declaration and the instantiation.
module \m+m (input a, output z);
  INV_X1 g1 (.A(a), .ZN(z));
endmodule
module top (input a, output z);
  \m+m u1 (.a(a), .z(z));
endmodule

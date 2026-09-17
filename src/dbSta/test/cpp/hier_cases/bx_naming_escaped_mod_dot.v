// TOP: top
// TECH: nangate45
// TARGETS: escaped_module, char_dot, depth_2
// CLUE: module named \m.n ; dot in a module name mimics a hierarchical
// reference at the definition site.
module \m.n (input a, output z);
  BUF_X1 g1 (.A(a), .Z(z));
endmodule
module top (input a, output z);
  \m.n u1 (.a(a), .z(z));
endmodule

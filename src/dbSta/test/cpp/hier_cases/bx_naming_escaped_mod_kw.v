// TOP: top
// TECH: nangate45
// TARGETS: escaped_module, keyword_module, depth_2
// CLUE: module named \module  (the keyword, escaped). If the writer emits
// it unescaped the output is unparseable.
module \module (input a, output z);
  INV_X1 g1 (.A(a), .ZN(z));
endmodule
module top (input a, output z);
  \module u1 (.a(a), .z(z));
endmodule

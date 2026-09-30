// TOP: t+p
// TECH: nangate45
// TARGETS: escaped_module, top_module, char_plus, depth_1
// CLUE: the TOP module itself is named \t+p ; link_design receives the
// unescaped identifier t+p.
module \t+p (input a, output z);
  INV_X1 g1 (.A(a), .ZN(z));
endmodule

// TOP: module
// TECH: nangate45
// TARGETS: escaped_module, keyword_module, top_module, depth_1
// CLUE: the TOP module itself is named \module ; both writers must emit
// "module \module (" -- variant of mod_kw that also hits the flat path.
module \module (input a, output z);
  INV_X1 g1 (.A(a), .ZN(z));
endmodule

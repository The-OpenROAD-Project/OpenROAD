// TOP: top
// TECH: nangate45
// TARGETS: orphan_module, module_dropped
// CLUE: module orphan is defined but never instantiated anywhere. Purely
// structural: does the hier writer keep the unreferenced module?
module orphan (input a, output y);
  INV_X1 g1 (.A(a), .ZN(y));
endmodule
module top (input x, output y);
  INV_X1 g1 (.A(x), .ZN(y));
endmodule

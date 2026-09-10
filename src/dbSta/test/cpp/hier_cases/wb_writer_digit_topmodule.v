// TOP: 1top
// TECH: nangate45
// TARGETS: needs_escape, leading_digit, top_module_name
// CLUE: writeModule spells the TOP module with cellVerilogName -> staToVerilog, whose
// CLUE: escape test is alnum/underscore only (VerilogNamespace.cc:108).  A digit-leading
// CLUE: escaped TOP name is therefore emitted bare in BOTH modes (unlike a submodule,
// CLUE: which flat mode flattens away).
module \1top (a, y);
  input a;
  output y;
  INV_X1 g (.A(a), .ZN(y));
endmodule

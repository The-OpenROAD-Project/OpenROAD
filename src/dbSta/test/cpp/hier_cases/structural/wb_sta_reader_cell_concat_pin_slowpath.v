// TOP: top
// TECH: nangate45
// TARGETS: liberty_fast_path, concat_pin, lef_mterm
// CLUE: hasScalarNamedPortRefs (VerilogReader.cc:475) requires every pin to be a
// CLUE: NamedPortRefScalarNet; a 1-bit concat {a} is a VerilogNetPortRefScalar, so the
// CLUE: whole instance leaves the liberty fast path and is linked as a module instance
// CLUE: (makePin over every LEF mterm, including VDD/VSS).
module top (a, y);
  input a;
  output y;
  wire n;
  INV_X1 g1 (.A({a}), .ZN(n));
  BUF_X1 g2 (.A(n), .Z(y));
endmodule

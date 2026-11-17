// TOP: top
// TECH: nangate45
// TARGETS: assign_emission, escaped_brackets, name_spelling_asymmetry
// CLUE: writeAssigns spells the PORT name with netVerilogName, not portVerilogName
// CLUE: (VerilogWriter.cc:460).  For an escaped bus-bit-looking output port that is the
// CLUE: LHS of an alias, the assign LHS and the port declaration are produced by two
// CLUE: different functions and could disagree.
module top (a, \z[0] );
  input a;
  output \z[0] ;
  wire n;
  INV_X1 g (.A(a), .ZN(n));
  assign \z[0]  = n;
endmodule

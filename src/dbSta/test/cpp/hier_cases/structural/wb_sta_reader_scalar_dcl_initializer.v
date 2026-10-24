// TOP: top
// TECH: nangate45
// TARGETS: net_decl_assignment, scalar, control
// CLUE: control for the bus case: a scalar net-declaration assignment survives because
// CLUE: size 1 is also what the missing declaration reports.  Verifies makeDcl keeps
// CLUE: assign args at all (VerilogReader.cc:313-334).
module top (a, y);
  input a;
  output y;
  wire n = a;
  INV_X1 g (.A(n), .ZN(y));
endmodule

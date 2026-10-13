// TOP: top
// TECH: nangate45
// TARGETS: same_scope_names, port_wire_redeclaration
// CLUE: non-ANSI ports with redundant wire declarations of the same names
// (legal per LRM); probes reader tolerance and writer round-trip.
module top (in1, o1);
  input in1;
  output o1;
  wire in1;
  wire o1;
  BUF_X1 g1 (.A(in1), .Z(o1));
endmodule

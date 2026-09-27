// TOP: top
// TECH: nangate45
// TARGETS: alias_output_from_output, top_level
// CLUE: output port o2 assigned FROM another output port o1 (o1 read back as RHS).

module top (input i, output o1, output o2);
  INV_X1 g0 (.A(i), .ZN(o1));
  assign o2 = o1;
endmodule

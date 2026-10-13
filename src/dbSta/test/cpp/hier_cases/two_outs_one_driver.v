// TOP: top
// TECH: nangate45
// TARGETS: alias_two_outputs, one_driver
// CLUE: two top output ports aliased to one gate driver via two assigns.

module top (input i, output o1, output o2);
  wire w;
  INV_X1 g0 (.A(i), .ZN(w));
  assign o1 = w;
  assign o2 = w;
endmodule

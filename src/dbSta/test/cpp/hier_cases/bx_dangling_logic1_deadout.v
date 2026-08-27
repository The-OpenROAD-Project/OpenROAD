// TOP: top
// TECH: nangate45
// TARGETS: tiecell_output_dead
// CLUE: LOGIC1_X1 tie cell whose only pin Z drives a dead wire. A tie cell with
//       zero observable effect is a prime candidate for silent deletion.
module top (x1, y);
  input x1;
  output y;
  wire dead;
  INV_X1 u1 (.A(x1), .ZN(y));
  LOGIC1_X1 u2 (.Z(dead));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: gate_output_dead, dead_wire_load
// CLUE: NAND2 u2 has both inputs driven but its output goes to wire dead that
//       feeds nothing. Purely structural: does u2 / dead survive the round trip?
module top (x1, x2, y);
  input x1;
  input x2;
  output y;
  wire dead;
  INV_X1 u1 (.A(x1), .ZN(y));
  NAND2_X1 u2 (.A1(x1), .A2(x2), .ZN(dead));
endmodule

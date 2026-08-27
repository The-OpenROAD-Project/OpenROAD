// TOP: top
// TECH: nangate45
// TARGETS: dead_assign_chain
// CLUE: assign chain d1 = x; d2 = d1; with no load on d2. Two-step dead alias
//       chain made only of assigns (no cells).
module top (x, y);
  input x;
  output y;
  wire d1;
  wire d2;
  INV_X1 u1 (.A(x), .ZN(y));
  assign d1 = x;
  assign d2 = d1;
endmodule

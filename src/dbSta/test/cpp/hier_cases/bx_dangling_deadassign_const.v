// TOP: top
// TECH: nangate45
// TARGETS: dead_assign, constant_rhs
// CLUE: assign dead = 1'b1; with no load on dead. Checks whether constant
// dead assigns are dropped like signal dead assigns.
module top (input x, output y);
  wire dead;
  assign dead = 1'b1;
  INV_X1 u1 (.A(x), .ZN(y));
endmodule

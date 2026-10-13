// TOP: top
// TECH: nangate45
// TARGETS: dead_assign
// CLUE: assign dead = x; wire dead has a driver but zero loads. Writers that
//       trace from outputs will never visit it.
module top (x, y);
  input x;
  output y;
  wire dead;
  INV_X1 u1 (.A(x), .ZN(y));
  assign dead = x;
endmodule

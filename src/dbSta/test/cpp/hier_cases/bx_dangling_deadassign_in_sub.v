// TOP: top
// TECH: nangate45
// TARGETS: dead_assign, inside_submodule
// CLUE: assign dead = a; lives INSIDE sub with no load. Scalar internal dead
// alias; the submodule cousin of the top-level dead assign.
module sub (input a, output y);
  wire dead;
  INV_X1 g1 (.A(a), .ZN(y));
  assign dead = a;
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .y(out1));
endmodule

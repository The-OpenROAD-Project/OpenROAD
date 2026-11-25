// TOP: top
// TECH: nangate45
// TARGETS: all_pins_unconnected, leaf_module
// CLUE: fully unconnected INV_X1 inside a submodule.
module sub (input a, output y);
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 u_alone ();
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .y(out1));
endmodule

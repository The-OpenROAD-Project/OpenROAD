// TOP: top
// TECH: nangate45
// TARGETS: undriven_net, live_cone, leaf_module
// CLUE: same undriven-net-feeding-live-logic shape, but inside a submodule.
module sub (input a, output y);
  wire und;
  AND2_X1 g1 (.A1(a), .A2(und), .ZN(y));
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .y(out1));
endmodule

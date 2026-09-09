// TOP: top
// TECH: nangate45
// TARGETS: sub_output_dangling, sequential_dead_logic
// CLUE: sub output q is driven by a DFF and left open at the parent, so the whole
// flop is unobservable. Does the flop survive both paths?
module sub (input a, input ck, output y, output q);
  INV_X1 g1 (.A(a), .ZN(y));
  DFF_X1 ff (.D(a), .CK(ck), .Q(q));
endmodule
module top (input in1, input ck, output out1);
  sub u1 (.a(in1), .ck(ck), .y(out1), .q());
endmodule

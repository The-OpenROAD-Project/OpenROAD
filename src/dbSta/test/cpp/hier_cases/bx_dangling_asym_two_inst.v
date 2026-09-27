// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, asymmetric_instances, dead_cone_inside_sub
// CLUE: two instances of sub; u1 connects port b, u2 leaves it open. b feeds a
// dead cone inside sub. Asymmetric dangling forces the writer to model the
// same module two different ways.
module sub (input a, input b, output y);
  wire dz;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(b), .ZN(dz));
endmodule
module top (input in1, input in2, output out1, output out2);
  sub u1 (.a(in1), .b(in2), .y(out1));
  sub u2 (.a(in2), .b(), .y(out2));
endmodule

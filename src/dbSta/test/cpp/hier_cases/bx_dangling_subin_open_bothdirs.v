// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, sub_output_dangling, mixed_open_pins
// CLUE: one instance leaves BOTH an input (b) and an output (z) open. Mixed
// directions on the same open instance: filler creation must not confuse them.
module sub (input a, input b, output y, output z);
  wire dz;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(b), .ZN(dz));
  BUF_X1 g3 (.A(a), .Z(z));
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .b(), .y(out1), .z());
endmodule

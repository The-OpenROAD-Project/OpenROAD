// TOP: top
// TECH: nangate45
// TARGETS: dead_gate_output, leaf_module
// CLUE: leaf contains INV g2 whose output net d feeds nothing; input side is live.
// LEC-invisible; does g2 survive both writers?
module sub (input a, output y);
  wire d;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(a), .ZN(d));
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .y(out1));
endmodule

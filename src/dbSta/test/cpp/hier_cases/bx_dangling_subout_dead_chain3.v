// TOP: top
// TECH: nangate45
// TARGETS: sub_output_dangling, dead_cone_depth3
// CLUE: sub output z is left open at the parent and is driven by a 3-gate chain
// inside sub. Measures how deep a dropped unobservable cone goes.
module sub (input a, output y, output z);
  wire c1;
  wire c2;
  INV_X1 g1 (.A(a), .ZN(y));
  BUF_X1 c_a (.A(a), .Z(c1));
  INV_X1 c_b (.A(c1), .ZN(c2));
  BUF_X1 c_c (.A(c2), .Z(z));
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .y(out1), .z());
endmodule

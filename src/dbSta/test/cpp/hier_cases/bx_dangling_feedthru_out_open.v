// TOP: top
// TECH: nangate45
// TARGETS: dangling_output, feedthrough_assign, empty_named_conn
// CLUE: sub output f is a pure feedthrough (assign f = a) left unconnected (.f()) at
// the parent. Scalar cousin of the known bus-slice feedthrough-drop finding.
module sub (input a, output y, output f);
  INV_X1 g1 (.A(a), .ZN(y));
  assign f = a;
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .y(out1), .f());
endmodule

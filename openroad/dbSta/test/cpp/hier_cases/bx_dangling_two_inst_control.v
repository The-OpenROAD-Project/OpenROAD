// TOP: top
// TECH: nangate45
// TARGETS: two_instances_control, no_dangling
// CLUE: control for gratuitous uniquification: two FULLY CONNECTED instances
// of sub, no dangling objects anywhere. If hier still emits sub twice, the
// uniquification is general hier-writer behavior, not dangling-triggered.
module sub (input a, output y);
  INV_X1 g1 (.A(a), .ZN(y));
endmodule
module top (input in1, input in2, output out1, output out2);
  sub u1 (.a(in1), .y(out1));
  sub u2 (.a(in2), .y(out2));
endmodule

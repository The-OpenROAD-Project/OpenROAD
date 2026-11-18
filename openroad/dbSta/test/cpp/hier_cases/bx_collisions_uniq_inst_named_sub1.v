// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, inst_named_sub1
// CLUE: brief-literal probe: module sub with instances NAMED sub_1 and
// sub_2; uniquify synthesizes sub_sub_2 -- benign under the <mod>_<inst>
// convention but probes suffix-style instance names.
module sub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  sub sub_1 (.a(in1), .z(o1));
  sub sub_2 (.a(in2), .z(o2));
endmodule

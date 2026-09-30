// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, inst_named_as_module
// CLUE: two instances of subm, the second NAMED subm; uniquify synthesizes
// subm_subm while the instance name subm shadows the module name in top.
module subm (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  subm k1 (.a(in1), .z(o1));
  subm subm (.a(in2), .z(o2));
endmodule

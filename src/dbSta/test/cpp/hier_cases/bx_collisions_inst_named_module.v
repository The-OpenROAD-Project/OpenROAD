// TOP: top
// TECH: nangate45
// TARGETS: same_scope_names, inst_named_as_module, single_inst
// CLUE: single instance of subm is NAMED subm (module vs instance namespaces
// are distinct per LRM); probes reader and both writers.
module subm (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, output o1);
  subm subm (.a(in1), .z(o1));
endmodule

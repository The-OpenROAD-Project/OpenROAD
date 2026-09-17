// TOP: top
// TECH: nangate45
// TARGETS: same_scope_names, leaf_inst_named_as_module
// CLUE: leaf INV instance named subx3 while module subx3 is instantiated in
// the same scope; legal per LRM (instance vs definition namespaces).
module subx3 (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  subx3 x (.a(in1), .z(o1));
  INV_X1 subx3 (.A(in2), .ZN(o2));
endmodule

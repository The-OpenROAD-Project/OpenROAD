// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, module_name_collision, uninstantiated_module
// CLUE: module sub_i2 exists but is never instantiated; if the linker keeps
// it, the uniquified name for instance i2 of sub still collides.
module sub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module sub_i2 (input a, output z);
  INV_X1 u1 (.A(a), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  sub i1 (.a(in1), .z(o1));
  sub i2 (.a(in2), .z(o2));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, module_name_collision, decl_order
// CLUE: same as uniq_vs_module_collide but module sub_i2 is declared BEFORE
// sub; brackets whether name-stealing depends on declaration order.
module sub_i2 (input a, output z);
  INV_X1 u1 (.A(a), .ZN(z));
endmodule

module sub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, input in2, input in3, output o1, output o2, output o3);
  sub i1 (.a(in1), .z(o1));
  sub i2 (.a(in2), .z(o2));
  sub_i2 u3 (.a(in3), .z(o3));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, module_name_collision, identical_content
// CLUE: same shape as uniq_vs_module_collide but the pre-existing sub_i2 has
// IDENTICAL content to sub; LEC cannot see a bad merge, so also check the
// hier output for duplicate module definitions.
module sub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module sub_i2 (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, input in2, input in3, output o1, output o2, output o3);
  sub i1 (.a(in1), .z(o1));
  sub i2 (.a(in2), .z(o2));
  sub_i2 u3 (.a(in3), .z(o3));
endmodule

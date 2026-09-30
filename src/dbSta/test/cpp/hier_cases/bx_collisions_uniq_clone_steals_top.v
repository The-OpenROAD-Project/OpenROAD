// TOP: sub_i2
// TECH: nangate45
// TARGETS: hier_uniquify, clone_name_equals_top_module
// CLUE: the TOP module is named sub_i2 -- exactly the name hier uniquify
// synthesizes for the second instance of sub; if the clone steals it the
// output has two modules named sub_i2 (one of them the top).
module sub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module sub_i2 (input in1, input in2, output o1, output o2);
  sub i1 (.a(in1), .z(o1));
  sub i2 (.a(in2), .z(o2));
endmodule

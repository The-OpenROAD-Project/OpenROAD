// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, module_name_collision, chained
// CLUE: sub (x2) wants uniquified name sub_i2 which is a real module that is
// ITSELF multiply instantiated (and being uniquified to sub_i2_j2); probes
// interacting renames.
module sub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module sub_i2 (input a, output z);
  INV_X1 u1 (.A(a), .ZN(z));
endmodule

module top (input in1, input in2, input in3, input in4,
            output o1, output o2, output o3, output o4);
  sub i1 (.a(in1), .z(o1));
  sub i2 (.a(in2), .z(o2));
  sub_i2 j1 (.a(in3), .z(o3));
  sub_i2 j2 (.a(in4), .z(o4));
endmodule

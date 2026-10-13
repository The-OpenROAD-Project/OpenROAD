// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, module_name_collision, victim_below_top
// CLUE: clone name sub_i2 is synthesized for instance i2 in top while the
// victim module sub_i2 (INV) is instantiated one level DOWN inside wrapper w;
// probes whether the name theft crosses hierarchy levels.
module sub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module sub_i2 (input a, output z);
  INV_X1 u1 (.A(a), .ZN(z));
endmodule

module wrapm (input a, output z);
  sub_i2 v1 (.a(a), .z(z));
endmodule

module top (input in1, input in2, input in3, output o1, output o2, output o3);
  sub i1 (.a(in1), .z(o1));
  sub i2 (.a(in2), .z(o2));
  wrapm w (.a(in3), .z(o3));
endmodule

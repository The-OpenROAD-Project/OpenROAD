// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, module_name_collision, below_top
// CLUE: clone name leafu_u is synthesized from instances BELOW top (mid1/u,
// mid2/u) while a user module leafu_u with different content is instantiated
// in top -- collision generated across hierarchy levels.
module leafu (input a, output z);
  BUF_X1 g (.A(a), .Z(z));
endmodule

module leafu_u (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module mid1 (input a, output z);
  leafu u (.a(a), .z(z));
endmodule

module mid2 (input a, output z);
  leafu u (.a(a), .z(z));
endmodule

module top (input in1, input in2, input in3, output o1, output o2, output o3);
  mid1 m1 (.a(in1), .z(o1));
  mid2 m2 (.a(in2), .z(o2));
  leafu_u k1 (.a(in3), .z(o3));
endmodule

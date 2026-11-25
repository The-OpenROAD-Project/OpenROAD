// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, resolution_suffix_collision, second_order
// CLUE: three parents instantiate leafu as u, forcing clones leafu_u and
// leafu_u_1 (the _1 comes from collision RESOLUTION); a user module named
// leafu_u_1 with different content already exists -- second-order collision.
module leafu (input a, output z);
  BUF_X1 g (.A(a), .Z(z));
endmodule

module leafu_u_1 (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module mid1 (input a, output z);
  leafu u (.a(a), .z(z));
endmodule

module mid2 (input a, output z);
  leafu u (.a(a), .z(z));
endmodule

module mid3 (input a, output z);
  leafu u (.a(a), .z(z));
endmodule

module top (input in1, input in2, input in3, input in4,
            output o1, output o2, output o3, output o4);
  mid1 m1 (.a(in1), .z(o1));
  mid2 m2 (.a(in2), .z(o2));
  mid3 m3 (.a(in3), .z(o3));
  leafu_u_1 k1 (.a(in4), .z(o4));
endmodule

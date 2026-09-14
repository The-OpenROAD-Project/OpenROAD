// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, synthesized_name_self_collision
// CLUE: leafu is instantiated as instance u in THREE different parents; the
// <module>_<inst> uniquify rule synthesizes leafu_u twice -- the synthesized
// names collide with each other.
module leafu (input a, output z);
  BUF_X1 g (.A(a), .Z(z));
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

module top (input in1, input in2, input in3, output o1, output o2, output o3);
  mid1 m1 (.a(in1), .z(o1));
  mid2 m2 (.a(in2), .z(o2));
  mid3 m3 (.a(in3), .z(o3));
endmodule

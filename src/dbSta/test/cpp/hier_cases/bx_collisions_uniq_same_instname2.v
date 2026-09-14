// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, synthesized_name_self_collision, bracket_variant
// CLUE: two-parent variant of uniq_same_instname3 (only one synthesized name
// needed, so no self-collision expected) -- brackets the trigger.
module leafu (input a, output z);
  BUF_X1 g (.A(a), .Z(z));
endmodule

module mid1 (input a, output z);
  leafu u (.a(a), .z(z));
endmodule

module mid2 (input a, output z);
  leafu u (.a(a), .z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  mid1 m1 (.a(in1), .z(o1));
  mid2 m2 (.a(in2), .z(o2));
endmodule

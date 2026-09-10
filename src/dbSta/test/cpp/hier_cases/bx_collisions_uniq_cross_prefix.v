// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, synthesized_name_cross_collision, module_prefix
// CLUE: psub instantiated as x_c2 synthesizes psub_x_c2; psub_x instantiated
// as c2 ALSO synthesizes psub_x_c2 -- two different-content modules (BUF vs
// INV) race for one synthesized name.
module psub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module psub_x (input a, output z);
  INV_X1 u1 (.A(a), .ZN(z));
endmodule

module top (input in1, input in2, input in3, input in4,
            output o1, output o2, output o3, output o4);
  psub a1 (.a(in1), .z(o1));
  psub x_c2 (.a(in2), .z(o2));
  psub_x c1 (.a(in3), .z(o3));
  psub_x c2 (.a(in4), .z(o4));
endmodule

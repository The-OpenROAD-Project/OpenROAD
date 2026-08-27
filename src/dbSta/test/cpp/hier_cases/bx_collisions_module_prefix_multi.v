// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, module_prefix, benign
// CLUE: modules psub / psub2 / psub_x are prefix-related and each is
// instantiated twice; synthesized names should stay distinct -- benign probe
// for prefix mishandling in uniquify.
module psub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module psub2 (input a, output z);
  INV_X1 u1 (.A(a), .ZN(z));
endmodule

module psub_x (input a, output z);
  XOR2_X1 u1 (.A(a), .B(a), .Z(z));
endmodule

module top (input in1, input in2, input in3, input in4, input in5, input in6,
            output o1, output o2, output o3, output o4, output o5, output o6);
  psub a1 (.a(in1), .z(o1));
  psub a2 (.a(in2), .z(o2));
  psub2 b1 (.a(in3), .z(o3));
  psub2 b2 (.a(in4), .z(o4));
  psub_x c1 (.a(in5), .z(o5));
  psub_x c2 (.a(in6), .z(o6));
endmodule

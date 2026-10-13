// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, net_named_as_clone_module
// CLUE: top WIRE named sub_i2 -- the very name hier uniquify synthesizes for
// the second instance of sub; module vs net namespaces differ (legal), but a
// naive uniquifier consulting one symbol table may rename or corrupt.
module sub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  wire sub_i2;
  sub i1 (.a(in1), .z(sub_i2));
  sub i2 (.a(in2), .z(o2));
  INV_X1 g1 (.A(sub_i2), .ZN(o1));
endmodule

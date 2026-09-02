// TOP: top
// TECH: nangate45
// TARGETS: escaped_module_name, hier_uniquify
// CLUE: escaped module \m/1 instantiated twice; hier uniquify must synthesize
// a name from an escaped base (m/1_i2?) and keep it escaped in the output.
module \m/1  (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  \m/1  i1 (.a(in1), .z(o1));
  \m/1  i2 (.a(in2), .z(o2));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: baseline, uniquify_probe
// CLUE: module sub instantiated twice, no name collisions; probes whether
// hier link/write uniquifies multiply-instantiated modules and what suffix
// convention it uses.
module sub (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  sub i1 (.a(in1), .z(o1));
  sub i2 (.a(in2), .z(o2));
endmodule

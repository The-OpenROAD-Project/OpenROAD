module flop_pair(input ck, output q0, q1);
  DFF_X1 ff0 (.CK(ck), .D(q1), .Q(q0));
  DFF_X1 ff1 (.CK(ck), .D(q0), .Q(q1));
endmodule
module nested(input clk1, output q0, q1, q2, q3);
  wire clock_a, clock_b;
  CLKBUF_X3 leaf_a (.A(clk1), .Z(clock_a));
  CLKBUF_X3 leaf_b (.A(clk1), .Z(clock_b));
  flop_pair bank_a (.ck(clock_a), .q0(q0), .q1(q1));
  flop_pair bank_b (.ck(clock_b), .q0(q2), .q1(q3));
endmodule

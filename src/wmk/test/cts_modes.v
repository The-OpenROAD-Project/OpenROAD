module cts_modes(input clk, d, output q0, q1, q2, q3);
  wire clock_a, clock_b;
  CLKBUF_X3 leaf_a (.A(clk), .Z(clock_a));
  CLKBUF_X3 leaf_b (.A(clk), .Z(clock_b));
  DFF_X1 ff0 (.CK(clock_a), .D(d), .Q(q0));
  DFF_X1 ff1 (.CK(clock_a), .D(d), .Q(q1));
  DFF_X1 ff2 (.CK(clock_b), .D(d), .Q(q2));
  DFF_X1 ff3 (.CK(clock_b), .D(d), .Q(q3));
endmodule

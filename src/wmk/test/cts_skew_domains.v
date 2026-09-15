module cts_skew_domains(input clk1, clk2, output q0, q1, q2, q3, h0, h1);
  wire clock_a, clock_b;
  CLKBUF_X3 leaf_a (.A(clk1), .Z(clock_a));
  CLKBUF_X3 leaf_b (.A(clk1), .Z(clock_b));
  DFF_X1 ff0 (.CK(clock_a), .D(q1), .Q(q0));
  DFF_X1 ff1 (.CK(clock_a), .D(q0), .Q(q1));
  DFF_X1 ff2 (.CK(clock_b), .D(q3), .Q(q2));
  DFF_X1 ff3 (.CK(clock_b), .D(q2), .Q(q3));
  DFF_X1 high0 (.CK(clk2), .D(h1), .Q(h0));
  DFF_X1 high1 (.CK(clk2), .D(h0), .Q(h1));
endmodule

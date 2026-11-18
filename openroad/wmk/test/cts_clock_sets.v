module cts_clock_sets(input clk1, clk2, select, output q0, q1, q2, q3);
  wire mixed_clock, clock_a, clock_b;
  MUX2_X1 mux (.A(clk1), .B(clk2), .S(select), .Z(mixed_clock));
  CLKBUF_X3 leaf_a (.A(clk1), .Z(clock_a));
  CLKBUF_X3 leaf_b (.A(mixed_clock), .Z(clock_b));
  DFF_X1 ff0 (.CK(clock_a), .D(q1), .Q(q0));
  DFF_X1 ff1 (.CK(clock_a), .D(q0), .Q(q1));
  DFF_X1 ff2 (.CK(clock_b), .D(q3), .Q(q2));
  DFF_X1 ff3 (.CK(clock_b), .D(q2), .Q(q3));
endmodule

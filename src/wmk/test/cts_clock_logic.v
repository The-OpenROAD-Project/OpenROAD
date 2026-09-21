module cts_clock_logic(input clk, enable_clock, output q0, q1, q2, q3);
  wire gated_clock, inverted_clock, clock_a, clock_b;
  INV_X1 invert (.A(gated_clock), .ZN(inverted_clock));
  CLKGATE_X1 gate (.CK(clk), .E(enable_clock), .GCK(gated_clock));
  CLKBUF_X3 leaf_a (.A(clk), .Z(clock_a));
  CLKBUF_X3 leaf_b (.A(gated_clock), .Z(clock_b));
  DFF_X1 ff0 (.CK(clock_a), .D(q1), .Q(q0));
  DFF_X1 ff1 (.CK(clock_a), .D(q0), .Q(q1));
  DFF_X1 ff2 (.CK(clock_b), .D(q3), .Q(q2));
  DFF_X1 ff3 (.CK(clock_b), .D(q2), .Q(q3));
endmodule

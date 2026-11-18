// Only negative-edge registers exist in this library: no base cell, no gates.
module top (a, clk, y);
  input a;
  input clk;
  output y;
  wire q1;

  DFF_N r1 (.D(a), .CK(clk), .Q(q1));
  DFF_N r2 (.D(q1), .CK(clk), .Q(y));
endmodule

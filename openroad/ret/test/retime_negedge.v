// A negative-edge register: retime refuses the design.
module top (a, clk, y);
  input a;
  input clk;
  output y;
  wire n0, q1;

  INV g0 (.A(a), .Y(n0));
  DFF_N r1 (.D(n0), .CK(clk), .Q(q1));
  INV g1 (.A(q1), .Y(y));
endmodule

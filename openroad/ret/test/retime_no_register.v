// Combinational only: a clock port and no register.
module top (a, b, clk, y);
  input a;
  input b;
  input clk;
  output y;
  wire n0;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));
  INV_X1 g1 (.A(n0), .ZN(y));
endmodule

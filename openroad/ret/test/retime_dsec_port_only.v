// rz drives only the output port z, so no edge of the retiming graph leaves
// i4.  Crossing i4 would take r1 from in front of it and put no register
// behind it: z would read a one cycle early.  i4 is never crossed, and r1
// moves forward only as far as its input.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, rst_n, clk, z);
  input a;
  input rst_n;
  input clk;
  output z;
  wire d1, q1, n1, n2, n3, n4;

  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  INV_X1 i1 (.A(q1), .ZN(n1));
  INV_X1 i2 (.A(n1), .ZN(n2));
  INV_X1 i3 (.A(n2), .ZN(n3));
  INV_X1 i4 (.A(n3), .ZN(n4));
  DFF_X1 rz (.D(n4), .CK(clk), .Q(z));
endmodule

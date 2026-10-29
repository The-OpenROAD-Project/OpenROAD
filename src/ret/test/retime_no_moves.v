// The only register sits between a gate fed by inputs and a gate driving an
// output; neither gate may be crossed, so retime finds no slides.
module top (a, b, clk, y);
  input a;
  input b;
  input clk;
  output y;
  wire n0, q1;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));
  DFF_X1 r1 (.D(n0), .CK(clk), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(y));
endmodule

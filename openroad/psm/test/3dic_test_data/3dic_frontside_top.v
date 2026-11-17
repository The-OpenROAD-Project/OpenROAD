// Use cells from Nangate45 library
module flop_chip_a (clk, d, q, VDD, VSS);
  input clk;
  input d;
  output q;
  inout VDD, VSS;

  wire n1, n2;

  DFF_X1 ff       (.D(d),  .CK(clk), .Q(n1), .QN());
  INV_X1 inv      (.A(n1), .ZN(n2));
  BUF_X1 buf_inst (.A(n2), .Z(q));
endmodule


module flop_chip_b (clk, d, q, VDD, VSS);
  input clk;
  input d;
  output q;
  inout VDD, VSS;

  wire n1, n2;

  DFF_X1 ff       (.D(d),  .CK(clk), .Q(n1), .QN());
  INV_X1 inv      (.A(n1), .ZN(n2));
  BUF_X1 buf_inst (.A(n2), .Z(q));
endmodule

// This file tells OpenROAD what is inside each chiplet: a flip-flop,
// an inverter, and a buffer. We use this file when creating the two DEF files.

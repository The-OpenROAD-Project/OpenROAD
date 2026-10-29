module top (d1, e1, d2, rst_n, clk, q1, qn2, q3);
  input d1;
  input e1;
  input d2;
  input rst_n;
  input clk;
  output q1;
  output qn2;
  output q3;

  // enable flop: becomes INV + 2 AND2 + OR2 in front of a DFF_P
  DFFE_PP ff1 (.D(d1), .EN(e1), .CK(clk), .Q(q1));
  // inverted-output flop: DFF_P with an INV behind it
  DFF_P_QN ff2 (.D(d2), .CK(clk), .QN(qn2));
  // asynchronous reset: left untouched
  DFF_PN0 ff3 (.D(d1), .RN(rst_n), .CK(clk), .Q(q3));
endmodule

module mixed_module (in0, in1, out0, out1);
  input in0, in1;
  output out0, out1;
  wire w0, w1, w2, w3;

  BUF_X1 b0 ( .A(in0), .Z(w0) );
  BUF_X1 b1 ( .A(in1), .Z(w1) );
  HM_100x100_1x1 m0 ( .I1(w0), .O1(w2) );
  HM_100x100_1x1 m1 ( .I1(w1), .O1(w3) );
  BUF_X1 b2 ( .A(w2), .Z(out0) );
  BUF_X1 b3 ( .A(w3), .Z(out1) );

endmodule

module keep_clustering_data2 (in0, in1, out0, out1);
  input in0, in1;
  output out0, out1;
  wire t0, t1, t2, t3;

  BUF_X1 g0 ( .A(in0), .Z(t0) );
  BUF_X1 g1 ( .A(in1), .Z(t1) );
  mixed_module mixed_module0 ( .in0(t0), .in1(t1), .out0(t2), .out1(t3) );
  BUF_X1 g2 ( .A(t2), .Z(out0) );
  BUF_X1 g3 ( .A(t3), .Z(out1) );

endmodule

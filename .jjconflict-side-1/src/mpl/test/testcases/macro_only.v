module macro_only ( );
  wire   w1, w2, w3, w4;
  wire   w11, w21, w31, w41;
  wire   w12, w22, w32, w42;

  HM_100x400_4x4 U1 ( .O1(w1), .O2(w2), .O3(w3), .O4(w4) ) ;
  HM_100x100_1x1 U2 ( .I1(w1), .O1(w11) ) ;
  HM_100x100_1x1 U3 ( .I1(w2), .O1(w21) ) ;
  HM_100x100_1x1 U4 ( .I1(w3), .O1(w31) ) ;
  HM_100x100_1x1 U5 ( .I1(w4), .O1(w41) ) ;
  HM_100x100_1x1 U7 ( .I1(w11) , .O1(w12) ) ;
  HM_100x100_1x1 U8 ( .I1(w21) , .O1(w22) ) ;
  HM_100x100_1x1 U9 ( .I1(w31) , .O1(w32) ) ;
  HM_100x100_1x1 U10 ( .I1(w41) , .O1(w42) ) ;
  HM_100x400_4x4 U6 ( .I1(w12), .I2(w22), .I3(w32), .I4(w42) ) ;

endmodule


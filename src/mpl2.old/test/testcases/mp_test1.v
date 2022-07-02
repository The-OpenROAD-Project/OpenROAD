
module mp_test1 ( i1, i2, i3, i4, o1, o2, o3, o4 );
  input i1, i2, i3, i4;
  output o1, o2, o3, o4;
  wire   w1, w2, w3, w4;
  wire   w11, w21, w31, w41;
  wire   w12, w22, w32, w42;

  HM_100x400_4x4 U1 ( .I1(i1), .I2(i2), .I3(i3), .I4(i4), .O1(w1), .O2(w2), .O3(w3), .O4(w4) ) ;
  HM_100x100_1x1 U2 ( .I1(w1), .O1(w11) ) ;
  HM_100x100_1x1 U3 ( .I1(w2), .O1(w21) ) ;
  HM_100x100_1x1 U4 ( .I1(w3), .O1(w31) ) ;
  HM_100x100_1x1 U5 ( .I1(w4), .O1(w41) ) ;
  HM_100x400_4x4 U6 ( .I1(w11), .I2(w21), .I3(w31), .I4(w41), .O1(w12), .O2(w22), .O3(w32), .O4(w42) ) ;
  HM_100x100_1x1 U7 ( .I1(w12), .O1(o1) ) ;
  HM_100x100_1x1 U8 ( .I1(n22), .O1(o2) ) ;
  HM_100x100_1x1 U9 ( .I1(n32), .O1(o3) ) ;
  HM_100x100_1x1 U10 ( .I1(n42), .O1(o4) ) ;
endmodule


module child (\bar[3] , \out[1] );
  input \bar[3] ;
  output \out[1] ;

  wire z0;
  wire z1;
  wire z2;
  wire z3;
  wire z4;
  wire z5;
  wire z6;
  wire z7;
  wire z8;
  wire z9;
  wire z10;
  wire z11;
  wire z12;
  wire z13;
  wire z14;
  wire z15;
  wire z16;
  wire z17;
  wire z18;
  wire z19;
  wire z20;
  wire z21;
  wire z22;
  wire z23;
  wire z24;
  wire z25;
  wire z26;
  wire z27;
  wire z28;
  wire z29;
  wire z30;
  wire z31;
  wire z32;
  wire z33;
  wire z34;

  BUF_X1 out_driver (.A(\bar[3] ), .Z(\out[1] ));

  BUF_X1 load0 (.A(\bar[3] ), .Z(z0));
  BUF_X1 load1 (.A(\bar[3] ), .Z(z1));
  BUF_X1 load2 (.A(\bar[3] ), .Z(z2));
  BUF_X1 load3 (.A(\bar[3] ), .Z(z3));
  BUF_X1 load4 (.A(\bar[3] ), .Z(z4));
  BUF_X1 load5 (.A(\bar[3] ), .Z(z5));
  BUF_X1 load6 (.A(\bar[3] ), .Z(z6));
  BUF_X1 load7 (.A(\bar[3] ), .Z(z7));
  BUF_X1 load8 (.A(\bar[3] ), .Z(z8));
  BUF_X1 load9 (.A(\bar[3] ), .Z(z9));
  BUF_X1 load10 (.A(\bar[3] ), .Z(z10));
  BUF_X1 load11 (.A(\bar[3] ), .Z(z11));
  BUF_X1 load12 (.A(\bar[3] ), .Z(z12));
  BUF_X1 load13 (.A(\bar[3] ), .Z(z13));
  BUF_X1 load14 (.A(\bar[3] ), .Z(z14));
  BUF_X1 load15 (.A(\bar[3] ), .Z(z15));
  BUF_X1 load16 (.A(\bar[3] ), .Z(z16));
  BUF_X1 load17 (.A(\bar[3] ), .Z(z17));
  BUF_X1 load18 (.A(\bar[3] ), .Z(z18));
  BUF_X1 load19 (.A(\bar[3] ), .Z(z19));
  BUF_X1 load20 (.A(\bar[3] ), .Z(z20));
  BUF_X1 load21 (.A(\bar[3] ), .Z(z21));
  BUF_X1 load22 (.A(\bar[3] ), .Z(z22));
  BUF_X1 load23 (.A(\bar[3] ), .Z(z23));
  BUF_X1 load24 (.A(\bar[3] ), .Z(z24));
  BUF_X1 load25 (.A(\bar[3] ), .Z(z25));
  BUF_X1 load26 (.A(\bar[3] ), .Z(z26));
  BUF_X1 load27 (.A(\bar[3] ), .Z(z27));
  BUF_X1 load28 (.A(\bar[3] ), .Z(z28));
  BUF_X1 load29 (.A(\bar[3] ), .Z(z29));
  BUF_X1 load30 (.A(\bar[3] ), .Z(z30));
  BUF_X1 load31 (.A(\bar[3] ), .Z(z31));
  BUF_X1 load32 (.A(\bar[3] ), .Z(z32));
  BUF_X1 load33 (.A(\bar[3] ), .Z(z33));
  BUF_X1 load34 (.A(\bar[3] ), .Z(z34));
endmodule

module top (\foo[7] , \out[0] );
  input \foo[7] ;
  output \out[0] ;

  child u_child (.\bar[3] (\foo[7] ),
                 .\out[1] (\out[0] ));
endmodule

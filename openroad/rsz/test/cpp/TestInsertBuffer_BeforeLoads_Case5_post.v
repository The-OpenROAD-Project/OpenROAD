module top ();

 wire net2;
 wire net1;
 wire n1;

 MOD0 mod0 (.Z(n1));
 MOD1 mod1 (.A(net1));
 MOD2 mod2 (.A(net1));
 MOD3 mod3 (.A(net2));
 BUF_X4 new01 (.A(net2),
    .Z(net1));
 BUF_X4 new12 (.A(n1),
    .Z(net2));
endmodule
module MOD0 (Z);
 output Z;


 BUF_X1 drvr0 (.Z(Z));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load0 (.A(A));
endmodule
module MOD2 (A);
 input A;


 BUF_X1 load1 (.A(A));
endmodule
module MOD3 (A);
 input A;


 BUF_X1 load2 (.A(A));
endmodule

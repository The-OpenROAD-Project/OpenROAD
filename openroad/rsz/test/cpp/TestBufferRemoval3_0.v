module top (clk, 
    in1,
    out1,
    out2);
 input clk;
 input in1;
 output out1;
 output out2;

 wire net1;
 wire net2;

 BUF_X2 buf (.A(net1),
    .Z(net2));
 BUF_X1 drvr (.A(in1),
    .Z(net1));
 BUF_X4 load (.A(net1),
    .Z(out1));
 MEM mem (.Z1(out2),
    .A1(net2),
    .A0(net1));
endmodule

module MEM (Z1,
    A1,
    A0);
 output Z1;
 input A1;
 input A0;


 BUF_X1 load0 (.A(A0));
 BUF_X1 load1 (.A(A1),
    .Z(Z1));
endmodule

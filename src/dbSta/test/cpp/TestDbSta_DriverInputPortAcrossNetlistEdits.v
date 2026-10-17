module top (clk,
    in2,
    out1);
 input clk;
 input in2;
 output out1;

 wire n0;
 wire n1;

 BUF_X1 b1 (.A(clk),
    .Z(n0));
 INV_X1 inv1 (.A(n0),
    .ZN(n1));
 NAND2_X1 nd1 (.A1(n1),
    .A2(in2),
    .ZN(out1));
endmodule

module top (clk,
    in,
    out0,
    out1,
    out2,
    out3,
    out4,
    out5,
    out6,
    out7,
    out8,
    out9,
    out10,
    out11,
    fan);
 input clk;
 input in;
 output out0;
 output out1;
 output out2;
 output out3;
 output out4;
 output out5;
 output out6;
 output out7;
 output out8;
 output out9;
 output out10;
 output out11;
 output fan;

 BUF_X1 split_drv (.A(in),
    .Z(fan));
 BUF_X1 load0 (.A(fan),
    .Z(out0));
 BUF_X1 load1 (.A(fan),
    .Z(out1));
 BUF_X1 load2 (.A(fan),
    .Z(out2));
 BUF_X1 load3 (.A(fan),
    .Z(out3));
 BUF_X1 load4 (.A(fan),
    .Z(out4));
 BUF_X1 load5 (.A(fan),
    .Z(out5));
 BUF_X1 load6 (.A(fan),
    .Z(out6));
 BUF_X1 load7 (.A(fan),
    .Z(out7));
 BUF_X1 load8 (.A(fan),
    .Z(out8));
 BUF_X1 load9 (.A(fan),
    .Z(out9));
 BUF_X1 load10 (.A(fan),
    .Z(out10));
 BUF_X1 load11 (.A(fan),
    .Z(out11));
endmodule

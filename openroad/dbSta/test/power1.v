module top (in1,
    in2,
    clk1,
    clk2,
    clk3,
    out);
 input in1;
 input in2;
 input clk1;
 input clk2;
 input clk3;
 output out;

 DFF_X1 r1 (.Q(r1q),
    .D(in1),
    .CK(clk1));
 DFF_X1 r2 (.Q(r2q),
    .D(in2),
    .CK(clk2));
 DFF_X1 r3 (.Q(out),
    .D(u2z),
    .CK(clk3));
 BUF_X1 u1 (.Z(u1z),
    .A(r2q));
 AND2_X1 u2 (.ZN(u2z),
    .A1(r1q),
    .A2(u1z));
endmodule

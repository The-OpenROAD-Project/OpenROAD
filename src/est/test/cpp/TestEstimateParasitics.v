module top (clk,
    scan_clk,
    d,
    q0,
    q1,
    qs);
 input clk;
 input scan_clk;
 input d;
 output q0;
 output q1;
 output qs;

 DFF_X1 reg0 (.D(d),
    .CK(clk),
    .Q(q0),
    .QN());
 DFF_X1 reg1 (.D(d),
    .CK(clk),
    .Q(q1),
    .QN());
 DFF_X1 scan_reg (.D(d),
    .CK(scan_clk),
    .Q(qs),
    .QN());
endmodule

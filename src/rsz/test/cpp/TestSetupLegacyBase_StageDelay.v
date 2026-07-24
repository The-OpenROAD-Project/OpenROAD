// Two-stage combinational chain for testing rankPathDrivers stage delay mode.
// BUF_X2 feeds BUF_X1; placing them at different coordinates creates a
// measurable wire delay on net n1 that the stage-delay ranking mode must add
// to the load delay of stage1.
module top (clk,
    in,
    out);
 input clk;
 input in;
 output out;

 wire n1;

 BUF_X2 stage1 (.A(in),
    .Z(n1));
 BUF_X1 stage2 (.A(n1),
    .Z(out));
endmodule

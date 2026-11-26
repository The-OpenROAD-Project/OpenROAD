// Sequential repair_power tests: two LVT flip-flops driven through RVT
// buffers so D-pin setup/hold checks are exercised by
// SequentialTimingEstimator.
module top (clk, d0, d1, q0, q1);
 input clk;
 input d0;
 input d1;
 output q0;
 output q1;
 wire n0;
 wire n1;

 BUF_X1 b0 (.A(d0), .Z(n0));
 DFF_X1_L ff0 (.D(n0), .CK(clk), .Q(q0));
 BUF_X1 b1 (.A(d1), .Z(n1));
 DFF_X1_L ff1 (.D(n1), .CK(clk), .Q(q1));
endmodule

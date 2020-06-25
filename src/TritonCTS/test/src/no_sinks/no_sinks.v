module test_no_sinks (clk);
 input clk;

 DFF_X1 ff1 (.D(_DISCONNECTED_1_),
    .CK(clk),
    .Q(_DISCONNECTED_2_),
    .QN(_DISCONNECTED_3_));
endmodule

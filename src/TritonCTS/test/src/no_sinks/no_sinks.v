module test_no_sinks (clk);
 input clk;

 DFF_X1 ff1 (.D(_DISCONNECTED_),
    .CK(clk),
    .Q(_DISCONNECTED_),
    .QN(_DISCONNECTED_));
endmodule

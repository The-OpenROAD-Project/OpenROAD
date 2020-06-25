module test_no_clk ();

 INV_X1 inv1 (.A(_DISCONNECTED_1_), .ZN(a));
 INV_X1 inv2 (.A(a), .ZN(_DISCONNECTED_2_));

endmodule

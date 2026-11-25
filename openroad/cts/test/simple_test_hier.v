module test_16_sinks (clk);
   input clk;
   flop_pair U1(clk);
   flop_pair U2(clk);
   flop_pair U3(clk);
   flop_pair U4(clk);
   flop_pair U5(clk);
   flop_pair U6(clk);
   flop_pair U7(clk);
   flop_pair U8(clk);
endmodule // test_16_sinks

module flop_pair(origclk);
   input origclk;
 DFF_X1 ff1 (.CK(origclk));
 DFF_X1 ff2 (.CK(origclk));
endmodule // flop_pair


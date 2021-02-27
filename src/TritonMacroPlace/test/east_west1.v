module mem2 (
	clk, 
	mem_out0, 
	mem_out1);
   input clk;
   output [6:0] mem_out0;
   output [6:0] mem_out1;
   
   fakeram45_64x7 mem0 (.clk(clk),
	                .rd_out(mem_out0));
   
   fakeram45_64x7 mem1 (.clk(clk),
	                .rd_out(mem_out1));
endmodule

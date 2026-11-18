module top (in, out, clk);
   input clk;
   input [3:0] in; 
   output [3:0] out; 

   bus4 bus1(.clk(clk), .in(in), .out(out));
endmodule

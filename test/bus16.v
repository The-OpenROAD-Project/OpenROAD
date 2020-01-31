module top (in, out);
   input [15:0] in; 
   output [15:0] out; 

   bus16 bus1(.in(in), .out(out));
endmodule

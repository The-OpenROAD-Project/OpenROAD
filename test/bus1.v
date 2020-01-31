module top (in, out);
   input [3:0] in; 
   output [3:0] out; 

   bus4 bus1(.in(in), .out(out));
endmodule

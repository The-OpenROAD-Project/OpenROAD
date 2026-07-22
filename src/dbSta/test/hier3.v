
module top (in, clk1, clk2, out, out2, bus_out);
   input in, clk1, clk2;
   output out, out2;
   output [3:0] bus_out;

   block1 b1 (.in(in), .clk(clk1), .out(b1out), .out2(out2));
   block2 b2 (.in(b1out), .clk(clk2), .out(out));
   block3 b3 (.in(out), .clk(clk1), .bus_out(bus_out));
endmodule // top

module block1 (in, clk, out, out2);
   input in, clk;
   output out, out2;

   BUF_X1 u1 (.A(in), .Z(u1out));
   DFF_X1 r1 (.D(u1out), .CK(clk), .Q(r1q));
   BUF_X1 u2 (.A(r1q), .Z(out));
   BUF_X1 u3 (.A(out), .Z(out2));
endmodule // block1

module block2 (in, clk, out, out2);
   input in, clk;
   output out, out2;

   BUF_X1 u1 (.A(in), .Z(u1out));
   DFF_X1 r1 (.D(u1out), .CK(clk), .Q(r1q));
   BUF_X1 u2 (.A(r1q), .Z(out));
   BUF_X1 u3 (.A(out), .Z(out2));
endmodule // block2

module block3 (in, clk, bus_out);
   input in, clk;
   output [3:0] bus_out;

   BUF_X1 u1 (.A(in), .Z(u1out));
   DFF_X1 r1 (.D(u1out), .CK(clk), .Q(r1q));
   BUF_X1 u2 (.A(r1q), .Z(bus_out[0]));
   BUF_X1 u3 (.A(bus_out[0]), .Z(bus_out[1]));
   BUF_X1 u4 (.A(bus_out[1]), .Z(bus_out[2]));
   BUF_X1 u5 (.A(bus_out[2]), .Z(bus_out[3]));
endmodule // block3


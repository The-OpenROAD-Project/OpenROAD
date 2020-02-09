module top (in, clk1, clk2, out);
   input in, clk1, clk2;
   output out;

   block1 b1 (.in(in), .clk(clk1), .out(b1out));
   block1 b2 (.in(b1out), .clk(clk2), .out(out));
endmodule // top

module block1 (in, clk, out);
   input in, clk;
   output out;

   snl_bufx1 u1 (.A(in), .Z(u1out));
   snl_ffqx1 r1 (.D(u1out), .CP(clk), .Q(r1q));
   snl_bufx1 u2 (.A(r1q), .Z(out));
endmodule // top

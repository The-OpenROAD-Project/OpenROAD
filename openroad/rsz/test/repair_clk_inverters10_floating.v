module top (d, q);
 input d;
 output q;
 wire clk_n;
 INV_X1 c1 (.A(), .ZN(clk_n));
 DFF_X1 r1 (.D(d), .CK(clk_n), .Q(q));
endmodule

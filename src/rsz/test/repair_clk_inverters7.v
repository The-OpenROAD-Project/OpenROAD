module top (clk, d, clk_n_o, q);
 input clk;
 input d;
 output clk_n_o;
 output q;
 wire clk_n;
 INV_X1 c1 (.A(clk), .ZN(clk_n));
 assign clk_n_o = clk_n;
 DFF_X1 r1 (.D(d), .CK(clk_n), .Q(q));
endmodule

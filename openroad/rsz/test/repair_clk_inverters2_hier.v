// The clock inverter's output net stays inside its module and descends
// into a child module (moditerm, no modbterm), the shape a synthesized
// gated clock takes. In the hierarchical flow the inverter's output pin
// then carries a dbModNet with register loads on its own segment.
module regs (clk_n, d, q1, q2);
 input clk_n;
 input d;
 output q1;
 output q2;
 DFF_X1 r1 (.D(d), .CK(clk_n), .Q(q1));
 DFF_X1 r2 (.D(d), .CK(clk_n), .Q(q2));
endmodule

module mid (clk_in, d, q0, q1, q2);
 input clk_in;
 input d;
 output q0;
 output q1;
 output q2;
 wire clk_n;
 INV_X1 c1 (.A(clk_in), .ZN(clk_n));
 DFF_X1 r0 (.D(d), .CK(clk_n), .Q(q0));
 regs u_regs (.clk_n(clk_n), .d(d), .q1(q1), .q2(q2));
endmodule

module top (clk, d, q0, q1, q2);
 input clk;
 input d;
 output q0;
 output q1;
 output q2;
 mid m1 (.clk_in(clk), .d(d), .q0(q0), .q1(q1), .q2(q2));
endmodule

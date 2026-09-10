// TOP: top
// TECH: nangate45
// TARGETS: swap_symmetric, two_instances, bus
// CLUE: Swap-symmetric bus pair: u1 gets (a,b), u2 gets (b,a) on 4-bit ports
// CLUE: and the child's function is not symmetric in its two inputs.

module top (a, b, y0, y1);
 input [3:0] a;
 input [3:0] b;
 output [3:0] y0;
 output [3:0] y1;
 sub u1 (.p(a), .q(b), .z(y0));
 sub u2 (.p(b), .q(a), .z(y1));
endmodule

module sub (p, q, z);
 input [3:0] p;
 input [3:0] q;
 output [3:0] z;
 AOI21_X1 g0 (.A(p[0]), .B1(q[0]), .B2(q[1]), .ZN(z[0]));
 AOI21_X1 g1 (.A(p[1]), .B1(q[1]), .B2(q[2]), .ZN(z[1]));
 AOI21_X1 g2 (.A(p[2]), .B1(q[2]), .B2(q[3]), .ZN(z[2]));
 AOI21_X1 g3 (.A(p[3]), .B1(q[3]), .B2(q[0]), .ZN(z[3]));
endmodule

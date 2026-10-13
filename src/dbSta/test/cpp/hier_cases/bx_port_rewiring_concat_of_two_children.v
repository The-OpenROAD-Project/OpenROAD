// TOP: top
// TECH: nangate45
// TARGETS: concat_port_conn, cross_child_concat
// CLUE: A child's 2-bit input port is fed by a concat that gathers one output
// CLUE: bit from each of two other child instances.

module top (a, y);
 input [3:0] a;
 output [1:0] y;
 wire c0, c1;
 g1 ua (.x(a[0]), .z(c0));
 g1 ub (.x(a[1]), .z(c1));
 h2 uc (.i({c1,c0}), .o(y));
endmodule

module g1 (x, z);
 input x;
 output z;
 INV_X1 n (.A(x), .ZN(z));
endmodule

module h2 (i, o);
 input [1:0] i;
 output [1:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 XOR2_X1 b1 (.A(i[1]), .B(i[0]), .Z(o[1]));
endmodule

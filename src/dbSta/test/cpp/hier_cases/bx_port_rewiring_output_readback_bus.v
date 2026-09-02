// TOP: top
// TECH: nangate45
// TARGETS: output_readback, bus, depth_1
// CLUE: Child output bus bits are read back inside the child to compute other
// CLUE: bits of the same output bus.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 input [3:0] i;
 output [3:0] o;
 INV_X1 g0 (.A(i[0]), .ZN(o[0]));
 INV_X1 g1 (.A(i[1]), .ZN(o[1]));
 XOR2_X1 g2 (.A(o[0]), .B(o[1]), .Z(o[2]));
 XNOR2_X1 g3 (.A(o[2]), .B(i[3]), .ZN(o[3]));
endmodule

// TARGETS: output_port_readback, bus, depth_2
// CLUE: Two bits of the submodule's OUTPUT bus are read back inside the same
// CLUE: submodule to build two other bits of the same output bus, at depth 2.
// CLUE: A port bit that is both a driver source and a boundary term is the
// CLUE: shape where the push-down that looks the internal net up by port name
// CLUE: can attach the wrong end.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 mid u (.i(a), .o(y));
endmodule

module mid (i, o);
 input [3:0] i;
 output [3:0] o;
 sub u (.i(i), .o(o));
endmodule

module sub (i, o);
 input [3:0] i;
 output [3:0] o;
 INV_X1 g3 (.A(i[3]), .ZN(o[3]));
 BUF_X1 g2 (.A(i[2]), .Z(o[2]));
 XOR2_X1 g1 (.A(o[3]), .B(i[1]), .Z(o[1]));
 XOR2_X1 g0 (.A(o[2]), .B(i[0]), .Z(o[0]));
endmodule

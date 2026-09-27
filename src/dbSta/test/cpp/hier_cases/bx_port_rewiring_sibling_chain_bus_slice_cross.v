// TOP: top
// TECH: nangate45
// TARGETS: sibling_chain, bus, slice_cross
// CLUE: Sibling chain where the intermediate bus is crossed by slices: upper
// CLUE: half of u1's output feeds the lower half of u2's input port.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 wire [3:0] m;
 pa u1 (.i(a), .o(m));
 pa u2 (.i({m[1:0],m[3:2]}), .o(y));
endmodule

module pa (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: sibling_chain, bus, perm, no_parent_logic
// CLUE: u1's 4-bit output bus feeds u2's input port through a scrambling
// CLUE: concat, with no logic at the parent at all.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 wire [3:0] m;
 sub u1 (.i(a), .o(m));
 sub u2 (.i({m[0],m[3],m[1],m[2]}), .o(y));
endmodule

module sub (i, o);
 input [3:0] i;
 output [3:0] o;
 INV_X1 b0 (.A(i[0]), .ZN(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 INV_X1 b2 (.A(i[2]), .ZN(o[2]));
 INV_X1 b3 (.A(i[3]), .ZN(o[3]));
endmodule

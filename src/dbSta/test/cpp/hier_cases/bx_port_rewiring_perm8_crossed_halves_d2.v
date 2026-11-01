// TOP: top
// TECH: nangate45
// TARGETS: perm_rev, bus8, crossed_halves, depth_2
// CLUE: 8-bit bus split into two 4-bit halves that are swapped at level 1 and
// CLUE: reversed inside the leaf: two independent bit permutations composed
// CLUE: through part-select port connections.

module top (a, y);
 input [7:0] a;
 output [7:0] y;
 l1 u (.i(a), .o(y));
endmodule

module l1 (i, o);
 input [7:0] i;
 output [7:0] o;
 h4 ulo (.i(i[7:4]), .o(o[3:0]));
 h4 uhi (.i(i[3:0]), .o(o[7:4]));
endmodule

module h4 (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[3]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[1]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule

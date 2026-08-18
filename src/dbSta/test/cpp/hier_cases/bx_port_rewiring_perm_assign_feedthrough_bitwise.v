// TOP: top
// TECH: nangate45
// TARGETS: perm_rev, assign_feedthrough, bit_assigns
// CLUE: PROBE around known finding 2: same reversal but as four scalar bit
// CLUE: assigns instead of one vector assign.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 rev u (.i(a), .o(y));
endmodule

module rev (i, o);
 input [3:0] i;
 output [3:0] o;
 assign o[0] = i[3];
 assign o[1] = i[2];
 assign o[2] = i[1];
 assign o[3] = i[0];
endmodule

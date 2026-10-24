// TOP: top
// TECH: nangate45
// TARGETS: perm_rev, assign_feedthrough, no_cells_in_child
// CLUE: PROBE around known finding 2: the child performs the whole 4-bit
// CLUE: reversal with one vector assign and contains no cells at all.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 rev u (.i(a), .o(y));
endmodule

module rev (i, o);
 input [3:0] i;
 output [3:0] o;
 assign o = {i[0],i[1],i[2],i[3]};
endmodule

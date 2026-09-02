// TOP: top
// TECH: nangate45
// TARGETS: perm_rev, assign_feedthrough, mixed_cells_and_assign
// CLUE: PROBE around known finding 2: half of the child output bus is a slice
// CLUE: feedthrough assign, the other half is driven by cells. If the assign
// CLUE: half is dropped, only two of four output bits lose their driver.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 rev u (.i(a), .o(y));
endmodule

module rev (i, o);
 input [3:0] i;
 output [3:0] o;
 assign o[1:0] = i[3:2];
 BUF_X1 b2 (.A(i[1]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule

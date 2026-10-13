// TOP: top
// TECH: nangate45
// TARGETS: assign_feedthrough, part_select_assign, halves_swapped
// CLUE: PROBE around known finding 2: the child is pure wiring, swapping the
// CLUE: two halves of the bus with two part-select assigns and no cells.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sw u (.i(a), .o(y));
endmodule

module sw (i, o);
 input [3:0] i;
 output [3:0] o;
 assign o[3:2] = i[1:0];
 assign o[1:0] = i[3:2];
endmodule

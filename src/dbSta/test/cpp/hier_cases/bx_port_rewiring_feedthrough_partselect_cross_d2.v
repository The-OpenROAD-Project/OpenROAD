// TOP: top
// TECH: nangate45
// TARGETS: assign_feedthrough, part_select_assign, depth_2
// CLUE: Bracketing variant of feedthrough_partselect_cross one level deeper:
// CLUE: the wiring-only module sits under an intermediate hierarchy level.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 mid u (.i(a), .o(y));
endmodule

module mid (i, o);
 input [3:0] i;
 output [3:0] o;
 sw u (.i(i), .o(o));
endmodule

module sw (i, o);
 input [3:0] i;
 output [3:0] o;
 assign o[3:2] = i[1:0];
 assign o[1:0] = i[3:2];
endmodule

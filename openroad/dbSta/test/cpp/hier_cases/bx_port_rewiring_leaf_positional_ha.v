// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, leaf_cell, liberty_pin_order, two_outputs
// CLUE: HA_X1 g (a, b, co, s): liberty order A,B,CO,S. Two inputs then two
// CLUE: outputs - bracket for the FA_X1 case with one fewer input.

module top (a, b, co, s);
 input a, b;
 output co, s;
 HA_X1 g (a, b, co, s);
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, leaf_cell, liberty_pin_order, two_outputs
// CLUE: FA_X1 g (a, b, ci, co, s): liberty order is A,B,CI,CO,S - the CARRY
// CLUE: output comes BEFORE the sum output. A reader that assumes outputs come
// CLUE: last, or sorts them, swaps co and s.

module top (a, b, ci, co, s);
 input a, b, ci;
 output co, s;
 FA_X1 g (a, b, ci, co, s);
endmodule

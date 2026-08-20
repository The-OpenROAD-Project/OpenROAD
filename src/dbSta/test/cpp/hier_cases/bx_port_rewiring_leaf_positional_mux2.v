// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, leaf_cell, liberty_pin_order, asymmetric
// CLUE: MUX2_X1 g (a, b, s, z): liberty order A,B,S,Z. All three inputs are
// CLUE: logically distinct, so any positional slip is LEC-visible.

module top (a, b, s, y);
 input a, b, s;
 output y;
 MUX2_X1 g (a, b, s, y);
endmodule

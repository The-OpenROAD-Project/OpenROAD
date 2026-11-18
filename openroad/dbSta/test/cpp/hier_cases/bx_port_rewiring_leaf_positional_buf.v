// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, leaf_cell, liberty_pin_order, two_pin
// CLUE: Minimal bracket for the positional leaf-cell binding order: BUF_X1 has
// CLUE: exactly two pins, A then Z in the liberty.

module top (a, y);
 input a;
 output y;
 BUF_X1 g (a, y);
endmodule

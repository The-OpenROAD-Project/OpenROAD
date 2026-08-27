// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, leaf_cell, liberty_pin_order
// CLUE: Positional connection to a library cell: INV_X1 g (a, y) must bind by
// CLUE: the liberty pin order A,ZN.

module top (a, y);
 input a;
 output y;
 INV_X1 g (a, y);
endmodule

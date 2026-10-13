// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, leaf_cell, liberty_pin_order
// CLUE: NAND2_X1 g (a, b, y): liberty order A1,A2,ZN. Swapping the inputs is
// CLUE: logically invisible, so a wrong ZN slot is the observable failure.

module top (a, b, y);
 input a, b;
 output y;
 NAND2_X1 g (a, b, y);
endmodule

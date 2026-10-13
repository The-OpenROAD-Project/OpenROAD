// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, leaf_cell, liberty_pin_order, asymmetric
// CLUE: AOI21_X1 g (a, b1, b2, y): liberty order A,B1,B2,ZN. A is the OR-side
// CLUE: input so it is not interchangeable with B1/B2.

module top (a, b, c, y);
 input a, b, c;
 output y;
 AOI21_X1 g (a, b, c, y);
endmodule

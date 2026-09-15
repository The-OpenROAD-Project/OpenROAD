// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, leaf_cell, scalar_pin
// CLUE: PROBE/bracket for the width-mismatch connection drop: a 2-bit net is
// CLUE: bound to the scalar pin of a LEAF cell instead of a module port.

module top (a, y);
 input [1:0] a;
 output y;
 INV_X1 g (.A(a), .ZN(y));
endmodule

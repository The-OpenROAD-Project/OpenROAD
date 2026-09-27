// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, leaf_cell, sequential, probe
// CLUE: PROBE: DFF_X1 g (d, ck, q, qn) - liberty order D,CK,Q,QN. Positional
// CLUE: binding of a sequential cell's clock slot.

module top (d, ck, q, qn);
 input d, ck;
 output q, qn;
 DFF_X1 g (d, ck, q, qn);
endmodule

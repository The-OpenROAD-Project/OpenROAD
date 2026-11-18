// TOP: top
// TECH: nangate45
// TARGETS: escaped_identity, net_canonicalization
// CLUE: net declared as \w1 (escaped) but referenced as plain w1 -- the LRM
// says they are the same identifier; a reader treating them as distinct
// leaves the g2 input undriven.
module top (input in1, output o1);
  wire \w1 ;
  INV_X1 g1 (.A(in1), .ZN(\w1 ));
  INV_X1 g2 (.A(w1), .ZN(o1));
endmodule

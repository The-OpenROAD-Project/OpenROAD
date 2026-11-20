// TOP: top
// TECH: nangate45
// TARGETS: escaped_lexes_plain, net, depth_1
// CLUE: net declared as \abc  which per LRM denotes the SAME identifier as
// plain abc; writer may normalize to abc (pass + structural note).
module top (input a, output z);
  wire \abc ;
  INV_X1 u1 (.A(a), .ZN(\abc ));
  INV_X1 u2 (.A(\abc ), .ZN(z));
endmodule

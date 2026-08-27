// TOP: top
// TECH: nangate45
// TARGETS: escaped_lexes_plain, module, depth_2
// CLUE: module declared as \sub  and instantiated as \sub ; identical to
// plain sub per LRM, checks reader unification and writer normalization.
module \sub (input a, output z);
  INV_X1 g1 (.A(a), .ZN(z));
endmodule
module top (input a, output z);
  \sub u1 (.a(a), .z(z));
endmodule

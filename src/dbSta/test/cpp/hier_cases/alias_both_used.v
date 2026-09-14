// TOP: top
// TECH: nangate45
// TARGETS: alias_both_names_loaded
// CLUE: net and its assign-alias BOTH drive gate pins; writer may collapse the alias and must keep both loads correct.

module top (input i, output o1, output o2);
  wire a, b;
  INV_X1 g0 (.A(i), .ZN(a));
  assign b = a;
  INV_X1 g1 (.A(a), .ZN(o1));
  INV_X1 g2 (.A(b), .ZN(o2));
endmodule

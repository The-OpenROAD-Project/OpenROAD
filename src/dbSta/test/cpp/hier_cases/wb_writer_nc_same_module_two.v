// TOP: top
// TECH: nangate45
// TARGETS: nc_filler, hier, instance_sort_order
// CLUE: control for the cross-module counter case: two open vector formals in ONE
// CLUE: module should declare and use _NC1.._NC4 consistently, and the assignment of
// CLUE: numbers should follow writeChildren's ASCII instance sort (La before Lz),
// CLUE: not declaration order.
module leaf (s, i, o);
  input s;
  input [1:0] i;
  output o;
  INV_X1 g (.A(s), .ZN(o));
endmodule

module top (a, b, y1, y2);
  input a;
  input b;
  output y1;
  output y2;
  leaf Lz (.s(a), .o(y1));
  leaf La (.s(b), .o(y2));
endmodule

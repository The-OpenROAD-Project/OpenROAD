// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, cross_kind_collision
// CLUE: top-level INSTANCE \x/y vs internal NET y inside hierarchy x; flat
// output gets instance x/y plus net x/y (one LRM namespace).
module subn2 (input a, output z);
  wire y;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(y), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  subn2 x (.a(in1), .z(o1));
  INV_X1 \x/y  (.A(in2), .ZN(o2));
endmodule

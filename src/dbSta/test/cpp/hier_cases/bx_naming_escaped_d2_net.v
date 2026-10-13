// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_plus, depth_2
// CLUE: net \w+w  INSIDE a submodule; flat writer must emit the prefixed
// path u1/w+w as a correctly escaped identifier.
module subn (input a, output z);
  wire \w+w ;
  INV_X1 g1 (.A(a), .ZN(\w+w ));
  INV_X1 g2 (.A(\w+w ), .ZN(z));
endmodule
module top (input a, output z);
  subn u1 (.a(a), .z(z));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_plus, depth_3
// CLUE: net \w+w  at hierarchy depth 3; flat path u1/u2/w+w must stay one
// escaped identifier.
module leafn (input a, output z);
  wire \w+w ;
  INV_X1 g1 (.A(a), .ZN(\w+w ));
  INV_X1 g2 (.A(\w+w ), .ZN(z));
endmodule
module midn (input a, output z);
  leafn u2 (.a(a), .z(z));
endmodule
module top (input a, output z);
  midn u1 (.a(a), .z(z));
endmodule

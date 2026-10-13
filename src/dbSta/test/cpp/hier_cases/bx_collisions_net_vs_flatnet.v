// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, flat_net_collision, depth_2
// CLUE: top-level NET \x/y collides with the flattened name of internal net
// y of hierarchy instance x; a merge would short two unrelated signals.
module subn (input a, output z);
  wire y;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(y), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  wire \x/y ;
  subn x (.a(in1), .z(o1));
  INV_X1 g3 (.A(in2), .ZN(\x/y ));
  INV_X1 g4 (.A(\x/y ), .ZN(o2));
endmodule

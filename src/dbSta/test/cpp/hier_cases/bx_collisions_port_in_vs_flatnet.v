// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, flat_net_collision
// CLUE: top INPUT PORT named \x/y collides with flattened internal net y of
// hierarchy x; a merge would rewire the port into the submodule cone.
module subn3 (input a, output z);
  wire y;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(y), .ZN(z));
endmodule

module top (input in1, input \x/y , output o1, output o2);
  subn3 x (.a(in1), .z(o1));
  INV_X1 g3 (.A(\x/y ), .ZN(o2));
endmodule

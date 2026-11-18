// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, cross_kind_collision
// CLUE: top-level NET \x/y vs INSTANCE y inside hierarchy x; in the flat
// output a net and an instance share the name x/y (one LRM namespace).
module subi (input a, output z);
  INV_X1 y (.A(a), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  wire \x/y ;
  subi x (.a(in1), .z(o1));
  INV_X1 g3 (.A(in2), .ZN(\x/y ));
  INV_X1 g4 (.A(\x/y ), .ZN(o2));
endmodule

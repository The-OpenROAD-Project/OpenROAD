// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, flat_net_collision, assign_alias
// CLUE: submodule net n is aliased to output port z by an assign; if the
// flattener keeps n as x/n it collides with the unrelated top escaped net
// \x/n , if it merges n into the parent net the collision disappears.
module suba (input a, output z);
  wire n;
  INV_X1 g1 (.A(a), .ZN(n));
  assign z = n;
endmodule

module top (input in1, input in2, output o1, output o2);
  wire \x/n ;
  suba x (.a(in1), .z(o1));
  INV_X1 g3 (.A(in2), .ZN(\x/n ));
  INV_X1 g4 (.A(\x/n ), .ZN(o2));
endmodule

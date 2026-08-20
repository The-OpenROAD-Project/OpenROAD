// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, flat_net_collision, collision_below_top
// CLUE: net collision generated below top: module mid2 contains escaped net
// \c/n AND hierarchy c with internal net n; both flatten to m/c/n.
module leafn2 (input a, output z);
  wire n;
  INV_X1 g1 (.A(a), .ZN(n));
  INV_X1 g2 (.A(n), .ZN(z));
endmodule

module mid2 (input a1, input a2, output z1, output z2);
  wire \c/n ;
  leafn2 c (.a(a1), .z(z1));
  INV_X1 g3 (.A(a2), .ZN(\c/n ));
  INV_X1 g4 (.A(\c/n ), .ZN(z2));
endmodule

module top (input in1, input in2, output o1, output o2);
  mid2 m (.a1(in1), .a2(in2), .z1(o1), .z2(o2));
endmodule

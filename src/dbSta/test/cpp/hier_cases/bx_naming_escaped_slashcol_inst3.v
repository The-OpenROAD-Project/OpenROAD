// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, slash_path_collision, depth_3
// CLUE: instance \x/y/z  at top plus real depth-3 hierarchy x.y.z; flat
// writer synthesizes path x/y/z -- deeper variant of the known collision.
module m2 (input a, output q);
  INV_X1 z (.A(a), .ZN(q));
endmodule
module m1 (input a, output q);
  m2 y (.a(a), .q(q));
endmodule
module top (input a, output o1, output o2);
  m1 x (.a(a), .q(o1));
  INV_X1 \x/y/z (.A(a), .ZN(o2));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, slash_path_collision, cross_namespace, depth_2
// CLUE: top INSTANCE \x/y  while hierarchy instance x contains internal
// NET y; flat output has instance x/y and net x/y -- same name, different
// namespaces (legal), checks the collision is per-namespace.
module subn2 (input a, output z);
  wire y;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(y), .ZN(z));
endmodule
module top (input a, output z, output w);
  subn2 x (.a(a), .z(w));
  INV_X1 \x/y (.A(a), .ZN(z));
endmodule

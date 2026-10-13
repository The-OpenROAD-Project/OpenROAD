// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, slash_path_collision, depth_2
// CLUE: instance \x/y  at top plus real hierarchy: instance x containing
// instance y; flat writer synthesizes path x/y and collides (known #3).
module subx (input a, output z);
  INV_X1 y (.A(a), .ZN(z));
endmodule
module top (input a, output z, output w);
  subx x (.a(a), .z(w));
  INV_X1 \x/y (.A(a), .ZN(z));
endmodule

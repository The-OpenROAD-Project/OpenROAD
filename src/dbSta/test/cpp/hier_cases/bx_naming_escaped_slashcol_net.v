// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, slash_path_collision, depth_2
// CLUE: top NET \x/y  plus hierarchy instance x whose module has internal
// net y; flat writer renames the internal net to x/y -- name collision on
// NETS (new variant of known instance collision).
module subx (input a, output z);
  wire y;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(y), .ZN(z));
endmodule
module top (input a, output z, output w);
  wire \x/y ;
  subx x (.a(a), .z(w));
  BUF_X1 g1 (.A(a), .Z(\x/y ));
  BUF_X1 g2 (.A(\x/y ), .Z(z));
endmodule

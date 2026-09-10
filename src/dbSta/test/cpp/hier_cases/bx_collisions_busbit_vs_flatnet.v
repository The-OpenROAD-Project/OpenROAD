// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, flat_net_collision, bus_bit
// CLUE: top escaped net \x/b[1] vs internal 2-bit bus b of hierarchy x; if
// the flat writer explodes the bus into per-bit nets named x/b[1] the names
// collide.
module subb (input a, output z);
  wire [1:0] b;
  INV_X1 g1 (.A(a), .ZN(b[0]));
  INV_X1 g2 (.A(b[0]), .ZN(b[1]));
  INV_X1 g3 (.A(b[1]), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  wire \x/b[1] ;
  subb x (.a(in1), .z(o1));
  INV_X1 g4 (.A(in2), .ZN(\x/b[1] ));
  INV_X1 g5 (.A(\x/b[1] ), .ZN(o2));
endmodule

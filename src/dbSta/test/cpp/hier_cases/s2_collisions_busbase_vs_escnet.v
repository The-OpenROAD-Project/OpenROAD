// TARGETS: flat_bus_base_collision, escaped_net, bus_regroup, depth_2
// CLUE: the flat writer regroups bus BITS back into a bus DECLARATION keyed on
// the base name (VerilogWriter.cc:283-305), so sub x's wire [1:0] b becomes
// `wire [1:0] \x/b ;`.  The victim here owns exactly that BASE name -- a top
// scalar \x/b -- not the bit name, so a case built on \x/b[1] misses it.
module subb (input a, output z);
  wire [1:0] b;
  INV_X1 g1 (.A(a), .ZN(b[0]));
  INV_X1 g2 (.A(b[0]), .ZN(b[1]));
  BUF_X1 g3 (.A(b[1]), .Z(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  wire \x/b ;
  subb x (.a(i1), .z(o1));
  INV_X1 g4 (.A(i2), .ZN(\x/b ));
  BUF_X1 g5 (.A(\x/b ), .Z(o2));
endmodule

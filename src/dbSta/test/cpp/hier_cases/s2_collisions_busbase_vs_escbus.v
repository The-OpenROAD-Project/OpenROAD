// TARGETS: flat_bus_base_collision, escaped_bus, bus_regroup, two_buses_one_name
// CLUE: both objects are BUSES.  Top declares wire [1:0] \x/b (sta name
// x\/b[0..1]); sub x declares wire [1:0] b, flattened to x/b[0..1].  The two
// sta names differ only by the escape character that staToVerilog drops
// (VerilogNamespace.cc:88-97), so writeWireDcls emits `wire [1:0] \x/b ;`
// TWICE and four distinct nets re-read as two.
module subb2 (input a, output z);
  wire [1:0] b;
  INV_X1 g1 (.A(a), .ZN(b[0]));
  INV_X1 g2 (.A(b[0]), .ZN(b[1]));
  BUF_X1 g3 (.A(b[1]), .Z(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  wire [1:0] \x/b ;
  subb2 x (.a(i1), .z(o1));
  INV_X1 g4 (.A(i2), .ZN(\x/b [0]));
  INV_X1 g5 (.A(\x/b [0]), .ZN(\x/b [1]));
  BUF_X1 g6 (.A(\x/b [1]), .Z(o2));
endmodule

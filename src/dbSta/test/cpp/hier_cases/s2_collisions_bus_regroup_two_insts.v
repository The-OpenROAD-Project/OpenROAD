// TARGETS: flat_bus_base_control, bus_regroup, two_instances, depth_2
// CLUE: control for the bus-base collisions.  Two instances of the same cell
// each hold a wire [1:0] b, so writeWireDcls (VerilogWriter.cc:283-305) must
// key its bus_ranges map on the FULL flattened base name and emit
// `wire [1:0] \x/b ;` and `wire [1:0] \y/b ;` separately.  A fix that made the
// regroup key the local bus name -- an easy way to "solve" the collision cases
// -- would short the two instances together here.
module subr (input a, output z);
  wire [1:0] b;
  INV_X1 g1 (.A(a), .ZN(b[0]));
  INV_X1 g2 (.A(b[0]), .ZN(b[1]));
  BUF_X1 g3 (.A(b[1]), .Z(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  subr x (.a(i1), .z(o1));
  subr y (.a(i2), .z(o2));
endmodule

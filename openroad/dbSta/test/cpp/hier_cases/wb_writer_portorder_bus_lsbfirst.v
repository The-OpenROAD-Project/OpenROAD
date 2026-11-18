// TOP: top
// TECH: nangate45
// TARGETS: bus_direction, msb_first_property, top_port
// CLUE: makeTopCell asks portMsbFirst(), which reads a dbBoolProperty cookie left by
// CLUE: read_verilog; if the cookie is missing groupBusPorts assumes MSB-first and
// CLUE: swaps from/to.  An LSB-first top bus [0:3] tests whether the cookie survives.
module top (a, z);
  input [0:3] a;
  output [0:3] z;
  INV_X1 g0 (.A(a[0]), .ZN(z[0]));
  INV_X1 g1 (.A(a[1]), .ZN(z[1]));
  INV_X1 g2 (.A(a[2]), .ZN(z[2]));
  INV_X1 g3 (.A(a[3]), .ZN(z[3]));
endmodule

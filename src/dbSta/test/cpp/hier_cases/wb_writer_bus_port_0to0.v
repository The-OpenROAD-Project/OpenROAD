// TOP: top
// TECH: nangate45
// TARGETS: bus_range, single_bit_bus, port_dcl
// CLUE: does a one-bit BUS port survive as a bus?  groupBusPorts builds a bus with
// CLUE: from==to==0 and writePortDcls prints network fromIndex/toIndex, so [0:0] should
// CLUE: round-trip -- but hasMembers() is then true, so any formal connection to it is
// CLUE: rewritten as a one-element concat.
module top (a, z);
  input [0:0] a;
  output [0:0] z;
  INV_X1 g (.A(a[0]), .ZN(z[0]));
endmodule

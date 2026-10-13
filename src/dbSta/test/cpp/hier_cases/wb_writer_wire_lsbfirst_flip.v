// TOP: top
// TECH: nangate45
// TARGETS: bus_range, wire_dcl, direction_flip
// CLUE: writeWireDcls rebuilds internal bus ranges as [max(index) : min(index)]
// CLUE: (VerilogWriter.cc:289-290) with no record of the declared direction, so an
// CLUE: LSB-first internal bus "wire [0:1] w" must come back as "wire [1:0] w".
module top (a, z);
  input [0:1] a;
  output [0:1] z;
  wire [0:1] w;
  INV_X1 g0 (.A(a[0]), .ZN(w[0]));
  INV_X1 g1 (.A(a[1]), .ZN(w[1]));
  BUF_X1 h0 (.A(w[0]), .Z(z[0]));
  BUF_X1 h1 (.A(w[1]), .Z(z[1]));
endmodule

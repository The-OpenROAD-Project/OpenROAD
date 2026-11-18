// TOP: top
// TECH: nangate45
// TARGETS: replication, assign_rhs, depth_0
// CLUE: Bracket for the {2{x}} reader rejection: replication in an ASSIGN rhs
// instead of a port connection. Does STA-0171 fire there too?
module top (x, z);
  input [1:0] x;
  output [3:0] z;
  wire [3:0] w;
  assign w = {2{x}};
  INV_X1 g0 (.A(w[0]), .ZN(z[0]));
  INV_X1 g1 (.A(w[1]), .ZN(z[1]));
  INV_X1 g2 (.A(w[2]), .ZN(z[2]));
  INV_X1 g3 (.A(w[3]), .ZN(z[3]));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: neg_bounds, internal_wire, desc_range, depth_0
// CLUE: Bracket for the nameless garbage-range wire finding: internal wire [2:-1]
// DESCENDING across zero (negwire_top used ascending [-1:2]).
module top (x, z);
  input [3:0] x;
  output [3:0] z;
  wire [2:-1] w;
  INV_X1 g0 (.A(x[0]), .ZN(w[-1]));
  INV_X1 g1 (.A(x[1]), .ZN(w[0]));
  INV_X1 g2 (.A(x[2]), .ZN(w[1]));
  INV_X1 g3 (.A(x[3]), .ZN(w[2]));
  BUF_X1 b0 (.A(w[-1]), .Z(z[0]));
  BUF_X1 b1 (.A(w[0]), .Z(z[1]));
  BUF_X1 b2 (.A(w[1]), .Z(z[2]));
  BUF_X1 b3 (.A(w[2]), .Z(z[3]));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: part_select_port, slice_of_slice, depth_1
// CLUE: Port connected to y[6:3] where y itself was assigned from a slice x[9:2]; two layers of part-select indirection.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z);
  input [11:0] x;
  output [3:0] z;
  wire [7:0] y;
  assign y = x[9:2];
  sub s (.a(y[6:3]), .y(z));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: bus_range_00, depth_1
// CLUE: [0:0] one-bit bus on child ports and internal wire; writer may collapse to scalar or emit stray [0] selects.
module sub (a, y);
  input [0:0] a;
  output [0:0] y;
  INV_X1 g0 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z);
  input [0:0] x;
  output [0:0] z;
  wire [0:0] w;
  sub s (.a(x), .y(w));
  BUF_X1 b0 (.A(w[0]), .Z(z[0]));
endmodule

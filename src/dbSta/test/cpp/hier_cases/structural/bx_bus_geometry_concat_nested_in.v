// TOP: top
// TECH: nangate45
// TARGETS: concat_port, nested_concat, depth_1
// CLUE: Nested concat {{a,b},c} as a port connection; flattening of nested concats must preserve order.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module top (a, b, c, z);
  input [1:0] a;
  input b;
  input c;
  output [3:0] z;
  sub s (.a({{a,b},c}), .y(z));
endmodule

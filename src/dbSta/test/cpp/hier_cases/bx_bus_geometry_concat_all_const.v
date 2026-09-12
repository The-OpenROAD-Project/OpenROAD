// TOP: top
// TECH: nangate45
// TARGETS: concat_port, const_bits, depth_1
// CLUE: Port connected to an all-constant concat {1'b0,1'b1,1'b0,1'b1}; plus one live input->output path so the design is not constant-only.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z, t);
  input x;
  output [3:0] z;
  output t;
  sub s (.a({1'b0,1'b1,1'b0,1'b1}), .y(z));
  BUF_X1 b (.A(x), .Z(t));
endmodule

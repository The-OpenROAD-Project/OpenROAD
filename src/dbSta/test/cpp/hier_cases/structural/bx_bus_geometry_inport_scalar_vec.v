// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, scalar_port, depth_1
// CLUE: Bracket for the STA-0200 drop-connection behavior: SCALAR child input
// connected to a 2-bit net; LRM truncates to x[0], OpenROAD may drop it.
module sub (a, y);
  input a;
  output y;
  INV_X1 g (.A(a), .ZN(y));
endmodule
module top (x, z, t);
  input [1:0] x;
  output z;
  output t;
  sub s (.a(x), .y(z));
  BUF_X1 b (.A(x[1]), .Z(t));
endmodule

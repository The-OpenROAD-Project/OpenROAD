// TOP: top
// TECH: nangate45
// TARGETS: const_port, scalar, depth_1
// CLUE: Bracket for zero_/one_ finding: DIRECT 1'b0 constant on a scalar child input (no concat). Does the writer still emit an undriven zero_ net?
module sub (a, y);
  input a;
  output y;
  INV_X1 g (.A(a), .ZN(y));
endmodule
module top (x, z, t);
  input x;
  output z;
  output t;
  sub s (.a(1'b0), .y(z));
  BUF_X1 b (.A(x), .Z(t));
endmodule

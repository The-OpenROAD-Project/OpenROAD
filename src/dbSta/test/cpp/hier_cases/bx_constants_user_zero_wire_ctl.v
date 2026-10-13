// TOP: top
// TECH: nangate45
// TARGETS: name_capture_control, zero_
// CLUE: CONTROL for bx_constants_user_zero_wire — user wire named zero_ with a
// real driver but NO literal constants anywhere; should round-trip clean,
// isolating the capture to literal+name coexistence.
module top (input a, output y, output yc);
  wire zero_;
  INV_X1 gi (.A(a), .ZN(zero_));
  BUF_X1 gb (.A(zero_), .Z(yc));
  INV_X1 g2 (.A(zero_), .ZN(y));
endmodule

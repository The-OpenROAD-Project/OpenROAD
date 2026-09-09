// TOP: top
// TECH: nangate45
// TARGETS: tie0, name_capture, zero_
// CLUE: user wire legitimately NAMED zero_ (driven by an inverter) coexists
// with a literal 1'b0 tie — the writer emits literal constants as an
// (undriven) net named zero_, so the tie may alias the user's driven net:
// wrong logic, not just a missing driver.
module top (input a, output y, output yc);
  wire zero_;
  INV_X1 gi (.A(a), .ZN(zero_));
  AND2_X1 g (.A1(zero_), .A2(1'b0), .ZN(y));
  BUF_X1 gb (.A(zero_), .Z(yc));
endmodule

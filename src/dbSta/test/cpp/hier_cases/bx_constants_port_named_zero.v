// TOP: top
// TECH: nangate45
// TARGETS: name_capture, zero_, port_name
// CLUE: top INPUT PORT legitimately named zero_ coexisting with a literal 1'b0
// tie — the reader's synthetic constant net zero_ may alias the port, wiring
// the tie pin to a live primary input (functional break at the boundary).
module top (input zero_, input a, output y, output yc);
  AND2_X1 g (.A1(a), .A2(1'b0), .ZN(y));
  BUF_X1 gb (.A(zero_), .Z(yc));
endmodule

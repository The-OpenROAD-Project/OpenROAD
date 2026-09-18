// TOP: top
// TECH: nangate45
// TARGETS: top_input_unused
// CLUE: top-level input nc is never referenced by any logic. The port must
//       survive round trip or the interface changes (boundary fidelity).
module top (x, nc, y);
  input x;
  input nc;
  output y;
  INV_X1 u1 (.A(x), .ZN(y));
endmodule

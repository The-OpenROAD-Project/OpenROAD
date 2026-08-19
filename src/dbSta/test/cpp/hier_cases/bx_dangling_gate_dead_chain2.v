// TOP: top
// TECH: nangate45
// TARGETS: gate_output_dead, dead_chain_depth2
// CLUE: two-gate dead chain: NOR2 u2 -> d1 -> INV u3 -> d2, d2 feeds nothing.
//       Tests whether a multi-gate unobservable cone survives intact.
module top (x1, x2, y);
  input x1;
  input x2;
  output y;
  wire d1;
  wire d2;
  INV_X1 u1 (.A(x1), .ZN(y));
  NOR2_X1 u2 (.A1(x1), .A2(x2), .ZN(d1));
  INV_X1 u3 (.A(d1), .ZN(d2));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: case_sensitivity, port, depth_1
// CLUE: Top ports d/D (inputs) and q/Q (outputs) differing only by case, with asymmetric functions so aliasing is LEC-visible.
module top (d, D, q, Q);
  input d, D;
  output q, Q;
  INV_X1 u1 (.A(d), .ZN(q));
  BUF_X1 u2 (.A(D), .Z(Q));
endmodule

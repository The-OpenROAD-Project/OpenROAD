// TOP: top
// TECH: nangate45
// TARGETS: gate_output_dead, explicit_empty_pin
// CLUE: NAND2 u2 output pin explicitly empty: .ZN(). Gate has no observable
//       effect; check whether the instance and the empty pin survive.
module top (x1, x2, y);
  input x1;
  input x2;
  output y;
  INV_X1 u1 (.A(x1), .ZN(y));
  NAND2_X1 u2 (.A1(x1), .A2(x2), .ZN());
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: gate_output_dead, omitted_pin
// CLUE: NAND2 u2 output pin ZN omitted from the named connection list.
//       Instance is pure dead logic; check survival and pin emission.
module top (x1, x2, y);
  input x1;
  input x2;
  output y;
  INV_X1 u1 (.A(x1), .ZN(y));
  NAND2_X1 u2 (.A1(x1), .A2(x2));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: top_input_bus_bit_unused
// CLUE: only xv[0] and xv[1] of 3-bit input xv are used; xv[2] dangles.
//       Bus must keep full [2:0] range even though the MSB is dead.
module top (xv, y);
  input [2:0] xv;
  output y;
  AND2_X1 u1 (.A1(xv[0]), .A2(xv[1]), .ZN(y));
endmodule

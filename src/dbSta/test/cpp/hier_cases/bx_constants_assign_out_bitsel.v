// TOP: top
// TECH: nangate45
// TARGETS: assign_const_output_port, bit_select
// CLUE: constant assigned to ONE BIT of an output bus (y[1]) while sibling
// bits are gate-driven — mixed driver kinds on one bus.
module top (input a, output [2:0] y);
  assign y[1] = 1'b0;
  INV_X1 g0 (.A(a), .ZN(y[0]));
  BUF_X1 g2 (.A(a), .Z(y[2]));
endmodule

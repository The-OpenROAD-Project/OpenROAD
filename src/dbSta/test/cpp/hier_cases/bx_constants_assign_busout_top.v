// TOP: top
// TECH: nangate45
// TARGETS: assign_const_output_port, bus_const
// CLUE: 4-bit constant assigned to a whole 4-bit top-level output bus; also a
// live gate output so the design is not all-constant.
module top (input a, output [3:0] yc, output y);
  assign yc = 4'hA;
  BUF_X1 g (.A(a), .Z(y));
endmodule

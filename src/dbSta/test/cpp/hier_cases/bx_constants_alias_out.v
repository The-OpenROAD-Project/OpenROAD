// TOP: top
// TECH: nangate45
// TARGETS: assign_const_wire, assign_const_output_port, alias
// CLUE: constant reaches a top output port through an ALIAS wire
// (w = 1'b0; yc = w) instead of directly — two assigns must collapse
// consistently.
module top (input a, output yc, output y);
  wire w;
  assign w = 1'b0;
  assign yc = w;
  BUF_X1 g (.A(a), .Z(y));
endmodule

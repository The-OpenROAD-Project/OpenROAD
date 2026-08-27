// TOP: top
// TECH: nangate45
// TARGETS: assign_const_output_port, top_level
// CLUE: constant assigned directly to a TOP-LEVEL output port; the port has no
// gate driver — writers sometimes lose port-level constant drivers.
module top (input a, output yc, output y);
  assign yc = 1'b1;
  INV_X1 g (.A(a), .ZN(y));
endmodule

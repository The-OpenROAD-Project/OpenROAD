// TOP: top
// TECH: nangate45
// TARGETS: output_port_readback
// CLUE: output port driven through an assign is also read as a gate input.

module top (input i, output o1, output o2);
  wire w;
  INV_X1 g0 (.A(i), .ZN(w));
  assign o1 = w;
  INV_X1 g1 (.A(o1), .ZN(o2));
endmodule

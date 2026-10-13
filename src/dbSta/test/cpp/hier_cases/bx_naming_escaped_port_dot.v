// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, char_dot, depth_1
// CLUE: top-level input named \p.q ; dot in a port name mimics a
// hierarchical pin path at the design boundary.
module top (input \p.q , output z);
  BUF_X1 u1 (.A(\p.q ), .Z(z));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: const_port, gate_pin, depth_0
// CLUE: Bracket for zero_/one_ finding at depth 0: leaf gate pin tied directly to 1'b1, no hierarchy at all.
module top (x, z, t);
  input x;
  output z;
  output t;
  INV_X1 g (.A(1'b1), .ZN(z));
  BUF_X1 b (.A(x), .Z(t));
endmodule

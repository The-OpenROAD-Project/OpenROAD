// TOP: top
// TECH: nangate45
// TARGETS: top_output_undriven
// CLUE: top output z has no driver at all. Violates the LEC-subject rule on
//       purpose; check both writers keep z as an (undriven) output port.
module top (x, y, z);
  input x;
  output y;
  output z;
  INV_X1 u1 (.A(x), .ZN(y));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: gate_input_undriven, gate_output_dead
// CLUE: AND2 u2 has input A2 tied to declared-but-undriven wire und AND its
//       output goes to dead wire. Doubly dangling, invisible to LEC coverage.
module top (x1, y);
  input x1;
  output y;
  wire und;
  wire dead;
  INV_X1 u1 (.A(x1), .ZN(y));
  AND2_X1 u2 (.A1(x1), .A2(und), .ZN(dead));
endmodule

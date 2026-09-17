// TOP: top
// TECH: nangate45
// TARGETS: dead_gate_island, undriven_input, dead_output
// CLUE: leaf gate g2: input und is undriven AND output d feeds nothing — a fully
// disconnected island, invisible to coverage. Purely structural check.
module sub (input a, output y);
  wire und;
  wire d;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(und), .ZN(d));
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .y(out1));
endmodule

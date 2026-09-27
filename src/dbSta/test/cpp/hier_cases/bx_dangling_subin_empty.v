// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, explicit_empty_named
// CLUE: submodule input b connected via explicit-empty .b(); b unused inside sub.
//       Writer may drop the port, the empty connection, or the whole formal.
module sub (a, b, y);
  input a;
  input b;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, y);
  input x;
  output y;
  sub u0 (.a(x), .b(), .y(y));
endmodule

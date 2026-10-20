// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, positional_trailing_empty_slot
// CLUE: positional instantiation with an empty TRAILING slot: sub u0 (x, y, );
//       last formal b is left dangling via an empty ordered connection.
module sub (a, y, b);
  input a;
  input b;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, y);
  input x;
  output y;
  sub u0 (x, y, );
endmodule

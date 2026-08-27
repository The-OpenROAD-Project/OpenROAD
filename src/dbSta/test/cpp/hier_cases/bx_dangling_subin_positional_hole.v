// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, positional_empty_slot
// CLUE: positional instantiation with an empty middle slot: sub u0 (x, , y);
//       the empty ordered connection is legal Verilog-2005 and leaves b dangling.
module sub (a, b, y);
  input a;
  input b;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, y);
  input x;
  output y;
  sub u0 (x, , y);
endmodule

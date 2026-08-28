// TOP: top
// TECH: nangate45
// TARGETS: positional_connection_control, sub_input_dangling_inside
// CLUE: control for the positional-hole cases: full positional list, no empty
//       slot; b gets a wire that is undriven-free (tied via x2). Isolates
//       whether OpenROAD rejects empty slots specifically or positional at all.
module sub (a, b, y);
  input a;
  input b;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, x2, y);
  input x;
  input x2;
  output y;
  sub u0 (x, x2, y);
endmodule

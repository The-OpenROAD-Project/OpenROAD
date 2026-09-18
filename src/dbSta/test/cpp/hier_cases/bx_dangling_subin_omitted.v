// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, omitted_named_connection
// CLUE: submodule input b simply omitted from named connection list; b unused
//       inside sub. Does the formal port b survive in hier output?
module sub (a, b, y);
  input a;
  input b;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, y);
  input x;
  output y;
  sub u0 (.a(x), .y(y));
endmodule

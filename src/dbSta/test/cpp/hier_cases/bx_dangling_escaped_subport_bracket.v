// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, escaped_port_bitselect_lookalike
// CLUE: sub input escaped as \q[0] (looks like a bit-select) is never
//       connected by the parent. If a writer drops the escaping, q[0] would
//       reference a nonexistent bus q.
module sub (a, \q[0] , y);
  input a;
  input \q[0] ;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, y);
  input x;
  output y;
  sub u0 (.a(x), .y(y));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, dead_cone_inside_sub
// CLUE: sub input b is unconnected at instantiation AND feeds a gate u2 inside
//       sub whose output dangles. Whole cone is invisible to LEC; does u2 survive?
module sub (a, b, y);
  input a;
  input b;
  output y;
  wire deadz;
  INV_X1 u1 (.A(a), .ZN(y));
  INV_X1 u2 (.A(b), .ZN(deadz));
endmodule

module top (x, y);
  input x;
  output y;
  sub u0 (.a(x), .b(), .y(y));
endmodule

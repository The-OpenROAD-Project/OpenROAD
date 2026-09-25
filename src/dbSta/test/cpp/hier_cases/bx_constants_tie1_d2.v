// TOP: top
// TECH: nangate45
// TARGETS: tie_high, depth_2
// CLUE: literal 1'b1 tie inside a submodule; flat path must uniquify the
// constant net, hier path must keep it inside the module.
module top (a, y);
  input a;
  output y;
  sub s1 (.i(a), .o(y));
endmodule

module sub (i, o);
  input i;
  output o;
  wire w0;
  AND2_X1 u1 (.A1(i), .A2(1'b1), .ZN(w0));
  INV_X1 u2 (.A(w0), .ZN(o));
endmodule

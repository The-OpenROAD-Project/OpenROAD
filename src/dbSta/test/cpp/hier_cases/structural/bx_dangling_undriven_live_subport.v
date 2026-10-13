// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, undriven_net_live_cone
// CLUE: sub input b left unconnected (.b()) but b IS used inside sub and its
//       cone reaches top output yu. LEC partial; check how each writer models b.
module sub (a, b, y);
  input a;
  input b;
  output y;
  AND2_X1 u1 (.A1(a), .A2(b), .ZN(y));
endmodule

module top (x1, y, yu);
  input x1;
  output y;
  output yu;
  INV_X1 u1 (.A(x1), .ZN(y));
  sub u0 (.a(x1), .b(), .y(yu));
endmodule

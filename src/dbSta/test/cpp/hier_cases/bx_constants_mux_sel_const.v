// TOP: top
// TECH: nangate45
// TARGETS: tie1, mux_select_pin
// CLUE: constant 1'b1 on a MUX2 select pin — the mux degenerates to a wire
// from B; const-prop may rewrite it, which is fine if equivalent.
module top (input a, input b, output y);
  MUX2_X1 m (.A(a), .B(b), .S(1'b1), .Z(y));
endmodule

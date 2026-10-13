// TOP: top
// TECH: nangate45
// TARGETS: name_capture, zero_, submodule_scope
// CLUE: a DRIVEN local wire named zero_ INSIDE a submodule coexists with a
// literal 1'b0 tie in the SAME submodule — hier writer re-emits the module
// body, so its synthetic zero_ constant net can capture the module-local
// user net (scope-local variant of bx_constants_user_zero_wire).
module sub (input a, output y, output yz);
  wire zero_;
  INV_X1 gi (.A(a), .ZN(zero_));
  BUF_X1 gb (.A(zero_), .Z(yz));
  AND2_X1 g (.A1(zero_), .A2(1'b0), .ZN(y));
endmodule

module top (input a, output y, output yz);
  sub s1 (.a(a), .y(y), .yz(yz));
endmodule

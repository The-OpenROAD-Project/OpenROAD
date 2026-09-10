// TOP: top
// TECH: nangate45
// TARGETS: assign_const_output_port, submodule
// CLUE: constant assigned to a SUBMODULE output port (module has no other
// logic); hier path must keep the assign inside sub, flat path must
// propagate the constant up.
module sub (output y);
  assign y = 1'b1;
endmodule

module top (input a, output y);
  wire t;
  sub s1 (.y(t));
  AND2_X1 g (.A1(a), .A2(t), .ZN(y));
endmodule

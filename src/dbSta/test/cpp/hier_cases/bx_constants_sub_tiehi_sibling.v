// TOP: top
// TECH: nangate45
// TARGETS: assign_const_output_port, submodule, sibling_logic
// CLUE: submodule output port tied hi via assign WHILE sibling logic in the
// same module drives another port — probes the duplicate-driver-on-driven-port
// writer bug family.
module sub (input a, output th, output y);
  assign th = 1'b1;
  INV_X1 g (.A(a), .ZN(y));
endmodule

module top (input a, output th, output y);
  sub s1 (.a(a), .th(th), .y(y));
endmodule

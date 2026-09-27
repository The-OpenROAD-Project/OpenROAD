// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, minimal_two_module_repro
// CLUE: smallest possible shape of the whole hazard: one scalar feedthrough submodule, one top wire named to sort before the top input, one scalar output. Predicted to break.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, output o);
  wire a_alias;
  ft u0 (.a(i), .y(a_alias));
  assign o = a_alias;
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, scalar_feedthrough, inverted_control
// CLUE: the PASSING concat_read_scalar_ft with ONLY the internal wire renamed from m to a_alias so it now sorts BEFORE the input port i. Predicted to break.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input k, output [1:0] o);
  wire a_alias;
  ft u0 (.a(i), .y(a_alias));
  assign o = {k, a_alias};
endmodule

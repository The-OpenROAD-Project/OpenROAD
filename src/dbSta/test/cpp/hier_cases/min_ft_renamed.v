// TOP: top
// TECH: nangate45
// TARGETS: scalar_feedthrough, name_selection_probe
// CLUE: structurally IDENTICAL to gp_no_bus_ft but with short net/port/instance names: if this passes while gp_no_bus_ft fails, the alias representative choice is name-order dependent.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input [3:0] b, input i, output [1:0] o, output p);
  wire m;
  ft u (.a(i), .y(m));
  assign o = {b[0], m};
  assign p = m;
endmodule

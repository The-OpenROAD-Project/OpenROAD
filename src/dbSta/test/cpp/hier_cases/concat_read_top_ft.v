// TOP: top
// TECH: nangate45
// TARGETS: concat_read_of_alias_wire, top_level, control
// CLUE: control for T2 without hierarchy: the alias wire is created by a top-level assign and read inside a top-level concat assign.

module top (input i, input k, output [1:0] o);
  wire m;
  assign m = i;
  assign o = {k, m};
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, top_level_only, no_hierarchy
// CLUE: scope test: the same badly sorted alias name with NO hierarchy at all, just a
// two-assign chain at top. Decides whether the bug is in flattening or in generic
// net-alias merging.

module top (input i, output o);
  wire a_alias;
  assign a_alias = i;
  assign o = a_alias;
endmodule

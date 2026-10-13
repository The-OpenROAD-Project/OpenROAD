// TOP: top
// TECH: nangate45
// TARGETS: no_cell_instances, submodule_feedthrough
// CLUE: entire design contains ZERO cell instances: a pure-assign feedthrough submodule under a pure-wiring top. Probes the degenerate all-alias netlist.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, output o);
  ft u0 (.a(i), .y(o));
endmodule

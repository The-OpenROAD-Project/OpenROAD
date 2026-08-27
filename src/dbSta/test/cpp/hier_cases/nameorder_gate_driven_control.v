// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, cell_driven_child, control
// CLUE: control for the name-order alias bug: identical shape but the child output is
// driven by a CELL, so no input/output alias merge happens. Predicted to pass.

module gsub (input a, output y);
  INV_X1 g0 (.A(a), .ZN(y));
endmodule

module top (input i, output o);
  wire a_alias;
  gsub u0 (.a(i), .y(a_alias));
  assign o = a_alias;
endmodule

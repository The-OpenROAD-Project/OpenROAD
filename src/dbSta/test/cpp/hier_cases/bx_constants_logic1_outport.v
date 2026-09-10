// TOP: top
// TECH: nangate45
// TARGETS: logic_cells, output_port_direct
// CLUE: LOGIC1_X1 cell output connected DIRECTLY to a top-level output port
// (no intermediate net name) — port-driving tie cell.
module top (input a, output yc, output y);
  LOGIC1_X1 l1 (.Z(yc));
  INV_X1 g (.A(a), .ZN(y));
endmodule

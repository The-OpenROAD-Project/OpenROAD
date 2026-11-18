// TOP: top
// TECH: nangate45
// TARGETS: z_literal, probe_reader
// CLUE: 1'bz connected to a cell pin — equivalent to leaving it unconnected
// for most tools; record what write_verilog emits structurally.
module top (input a, output y);
  OR2_X1 g1 (.A1(a), .A2(1'bz), .ZN(y));
endmodule

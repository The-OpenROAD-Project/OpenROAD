// TOP: top
// TECH: nangate45
// TARGETS: x_literal, probe_reader
// CLUE: 1'bx connected to a cell pin — reader may accept it; the oracle may
// not; record what write_verilog emits for the x connection either way.
module top (input a, output y);
  OR2_X1 g1 (.A1(a), .A2(1'bx), .ZN(y));
endmodule

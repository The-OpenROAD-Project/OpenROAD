// TOP: top
// TECH: nangate45
// TARGETS: unsized_literal, probe_reader
// CLUE: unsized 'b0 (32-bit) connected to a 1-bit cell pin — legal Verilog
// with truncation; probes whether the reader accepts unsized literals in
// port context.
module top (input a, output y);
  OR2_X1 g1 (.A1(a), .A2('b0), .ZN(y));
endmodule

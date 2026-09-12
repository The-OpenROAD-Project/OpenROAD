// TOP: top
// TECH: nangate45
// TARGETS: unsized_literal, probe_reader
// CLUE: unsized 'd1 connected to a 1-bit cell pin — LSB is 1, upper bits
// truncate; probes reader handling of unsized decimal literals.
module top (input a, output y);
  AND2_X1 g1 (.A1(a), .A2('d1), .ZN(y));
endmodule

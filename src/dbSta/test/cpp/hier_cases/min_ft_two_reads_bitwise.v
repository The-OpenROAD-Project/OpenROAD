// TOP: top
// TECH: nangate45
// TARGETS: scalar_feedthrough, two_readers, bit_assign_and_direct
// CLUE: same as min_ft_two_reads_concat but the output bus is driven by per-bit assigns instead of a concat.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input k, output [1:0] o, output p);
  wire m;
  ft u0 (.a(i), .y(m));
  assign o[1] = k;
  assign o[0] = m;
  assign p = m;
endmodule

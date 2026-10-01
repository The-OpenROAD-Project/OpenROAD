// TOP: top
// TECH: nangate45
// TARGETS: bus_range, zero_seeded_range, int_max
// CLUE: because the range accumulator is seeded at 0, a single-bit internal bus at
// CLUE: INT_MAX must be emitted as "wire [2147483647:0] w" -- a two-billion-bit
// CLUE: declaration produced from one real wire.
// CLUE: structural-only, and not because LEC errors on it: the two billion extra
// CLUE: bits are undriven and unread, so the emitted netlist really is equivalent
// CLUE: and a prover that gets that far says "proved". Whether it gets that far
// CLUE: depends on how much memory the host has to allocate 2^31 bit entries, so
// CLUE: as a conformance case its verdict tracked the machine rather than the
// CLUE: defect. declared_nets catches the erased range on every machine.
module top (a, z);
  input a;
  output z;
  wire [2147483647:2147483647] w;
  INV_X1 g (.A(a), .ZN(w[2147483647]));
  BUF_X1 h (.A(w[2147483647]), .Z(z));
endmodule

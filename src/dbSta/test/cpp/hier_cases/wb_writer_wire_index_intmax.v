// TOP: top
// TECH: nangate45
// TARGETS: bus_range, zero_seeded_range, int_max
// CLUE: because the range accumulator is seeded at 0, a single-bit internal bus at
// CLUE: INT_MAX must be emitted as "wire [2147483647:0] w" -- a two-billion-bit
// CLUE: declaration produced from one real wire.
module top (a, z);
  input a;
  output z;
  wire [2147483647:2147483647] w;
  INV_X1 g (.A(a), .ZN(w[2147483647]));
  BUF_X1 h (.A(w[2147483647]), .Z(z));
endmodule

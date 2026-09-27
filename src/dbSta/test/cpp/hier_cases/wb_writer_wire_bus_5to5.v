// TOP: top
// TECH: nangate45
// TARGETS: bus_range, wire_dcl, zero_seeded_range
// CLUE: bus_ranges[bus_name] is a value-initialised std::pair<int,int>, so index 0 is
// CLUE: folded into every range: a single-bit internal bus at index 5 ("wire [5:5] w")
// CLUE: must be re-emitted as "wire [5:0] w" with five phantom bits.
module top (a, z);
  input a;
  output z;
  wire [5:5] w;
  INV_X1 g (.A(a), .ZN(w[5]));
  BUF_X1 h (.A(w[5]), .Z(z));
endmodule

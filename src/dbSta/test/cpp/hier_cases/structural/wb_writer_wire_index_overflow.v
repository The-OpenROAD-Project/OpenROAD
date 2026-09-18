// TOP: top
// TECH: nangate45
// TARGETS: bus_range, stoi_overflow, robustness
// CLUE: parseBusName indexes with std::stoi and nobody catches std::out_of_range
// CLUE: (ParseBus.cc:95, VerilogNamespace.cc:61); a bus bit index above INT_MAX should
// CLUE: reach it as an uncaught exception rather than a diagnostic.
module top (a, z);
  input a;
  output z;
  wire [2147483648:2147483648] w;
  INV_X1 g (.A(a), .ZN(w[2147483648]));
  BUF_X1 h (.A(w[2147483648]), .Z(z));
endmodule

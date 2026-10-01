// TOP: top
// TECH: nangate45
// TARGETS: bus_range, stoi_out_of_range, robustness, writer_crash
// CLUE: netVerilogName and writeWireDcls both re-parse a bus-bit net name with
// CLUE: std::stoi (ParseBus.cc:95) and nobody catches std::out_of_range.  An
// CLUE: out-of-range bit select is legal Verilog (yields x); if the reader materialises
// CLUE: a net literally named w[9999999999] the writer must throw on it.
module top (a, z);
  input a;
  output z;
  wire [1:0] w;
  INV_X1 g (.A(a), .ZN(w[0]));
  BUF_X1 h (.A(w[9999999999]), .Z(z));
endmodule

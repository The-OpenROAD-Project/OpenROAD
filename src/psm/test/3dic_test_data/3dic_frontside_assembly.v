// read_3dbx reads this file to connect chipA and chipB.
// The top module below instantiates the two chiplets and connects their ports.
// The chiplet modules here list only their ports and not the internal cells.
// Leaving the cell instantiations out of this file avoids the "module not found" warnings

module flop_chip_a (clk, d, q, VDD, VSS);
  input clk, d;
  output q;
  inout VDD, VSS;
endmodule

module flop_chip_b (clk, d, q, VDD, VSS);
  input clk, d;
  output q;
  inout VDD, VSS;
endmodule

module top (clk_top, in_top, out_top, VDD, VSS);
  // _top marks a port of the top module; it does not describe its physical location.
  input clk_top;
  input in_top;
  output out_top;
  inout VDD, VSS;
  wire bridge;

  flop_chip_a chipA (
    .clk(clk_top), .d(in_top), .q(bridge),   // defines how 2 chiplets are connected
    .VDD(VDD), .VSS(VSS)
  );

  flop_chip_b chipB (
    .clk(clk_top), .d(bridge), .q(out_top),  // defines how 2 chiplets are connected
    .VDD(VDD), .VSS(VSS)
  );
endmodule

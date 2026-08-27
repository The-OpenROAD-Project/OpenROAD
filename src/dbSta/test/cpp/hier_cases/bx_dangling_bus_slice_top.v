// TOP: top
// TECH: nangate45
// TARGETS: dangling_bus_slice, internal_bus
// CLUE: internal bus w[3:0]: only w[0] is driven and read; w[3:1] never touched.
// Does the bus survive as [3:0] or shrink/explode?
module top (input in1, output out1);
  wire [3:0] w;
  assign w[0] = in1;
  INV_X1 g1 (.A(w[0]), .ZN(out1));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: 4bit_dff_bank_in_submodule, bus_ports_on_sequential_boundary
// CLUE: a 4-bit register bank in a submodule with bus D and Q ports; bit-blast
// of the bus must keep every flop bit aligned.

module bank (input [3:0] d, input ck, output [3:0] q);
  DFF_X1 f0 (.D(d[0]), .CK(ck), .Q(q[0]));
  DFF_X1 f1 (.D(d[1]), .CK(ck), .Q(q[1]));
  DFF_X1 f2 (.D(d[2]), .CK(ck), .Q(q[2]));
  DFF_X1 f3 (.D(d[3]), .CK(ck), .Q(q[3]));
endmodule

module top (input [3:0] d, input ck, output [3:0] q);
  bank u (.d(d), .ck(ck), .q(q));
endmodule

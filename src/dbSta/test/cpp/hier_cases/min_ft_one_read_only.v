// TOP: top
// TECH: nangate45
// TARGETS: scalar_feedthrough, single_reader, control
// CLUE: control: gp_no_bus_ft with the second reader removed (only the concat read of the feedthrough wire survives).

module ft (input in_single, output out_single);
  assign out_single = in_single;
endmodule

module top (input [3:0] top_in_bus, input top_in_single, output [1:0] top_out_bus);
  wire sub_out_single;
  ft sub_inst (.in_single(top_in_single), .out_single(sub_out_single));
  assign top_out_bus = {top_in_bus[0], sub_out_single};
endmodule

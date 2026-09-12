// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, scalar_feedthrough
// CLUE: min_ft_one_read_only with ONLY the top internal wire renamed so it sorts AFTER the driving top input port (zub_out_single > top_in_single). Predicted to pass.

module ft (input in_single, output out_single);
  assign out_single = in_single;
endmodule

module top (input [3:0] top_in_bus, input top_in_single, output [1:0] top_out_bus);
  wire zub_out_single;
  ft sub_inst (.in_single(top_in_single), .out_single(zub_out_single));
  assign top_out_bus = {top_in_bus[0], zub_out_single};
endmodule

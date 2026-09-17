// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, scalar_feedthrough
// CLUE: min_ft_one_read_only with ONLY the driving top input port renamed so it sorts BEFORE the internal wire (aop_in_single < sub_out_single). Predicted to pass.

module ft (input in_single, output out_single);
  assign out_single = in_single;
endmodule

module top (input [3:0] top_in_bus, input aop_in_single, output [1:0] top_out_bus);
  wire sub_out_single;
  ft sub_inst (.in_single(aop_in_single), .out_single(sub_out_single));
  assign top_out_bus = {top_in_bus[0], sub_out_single};
endmodule

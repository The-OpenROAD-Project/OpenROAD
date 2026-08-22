// TOP: top
// TECH: nangate45
// TARGETS: delta_debug, scalar_feedthrough_only
// CLUE: ladder on getports_wholein: the sub's BUS feedthrough port pair is removed, leaving only the scalar feedthrough whose wire is read twice at top.

module sub_module (input in_single, output out_single);
  assign out_single = in_single;
endmodule

module top (input [3:0] top_in_bus, input top_in_single,
            output [1:0] top_out_bus, output top_out_single);
  wire sub_out_single;
  sub_module sub_inst (.in_single(top_in_single), .out_single(sub_out_single));
  assign top_out_bus = {top_in_bus[0], sub_out_single};
  assign top_out_single = sub_out_single;
endmodule

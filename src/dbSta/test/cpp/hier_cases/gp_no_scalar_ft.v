// TOP: top
// TECH: nangate45
// TARGETS: delta_debug, bus_feedthrough_only
// CLUE: ladder on getports_wholein: the sub's SCALAR feedthrough port pair is removed, leaving only the bus-slice feedthrough.

module sub_module (input [3:0] in_bus, output [1:0] out_bus);
  assign out_bus = in_bus[3:2];
endmodule

module top (input [3:0] top_in_bus, output [1:0] top_out_bus,
            output top_out_single);
  wire [1:0] sub_out_bus;
  sub_module sub_inst (.in_bus(top_in_bus), .out_bus(sub_out_bus));
  assign top_out_bus = {top_in_bus[0], sub_out_bus[1]};
  assign top_out_single = sub_out_bus[0];
endmodule

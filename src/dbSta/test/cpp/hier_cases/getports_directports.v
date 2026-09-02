// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, sliced_instance_input, direct_port_out
// CLUE: get_ports1 shape with both sub outputs wired straight onto top output port slices: if this still fails, the internal top wire is irrelevant.

module sub_module (input [3:0] in_bus, input in_single,
                   output [1:0] out_bus, output out_single);
  assign out_bus = in_bus[3:2];
  assign out_single = in_single;
endmodule

module top (input [7:0] top_in_bus, input top_in_single,
            output [2:0] top_out_bus);
  sub_module sub_inst (.in_bus(top_in_bus[3:0]), .in_single(top_in_single),
                       .out_bus(top_out_bus[1:0]), .out_single(top_out_bus[2]));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: delta_debug, no_unused_input_bits
// CLUE: ladder on getports_wholein: the sub's input bus is exactly as wide as its output (no unused in_bus[1:0], no RHS part select).

module sub_module (input [1:0] in_bus, input in_single,
                   output [1:0] out_bus, output out_single);
  assign out_bus = in_bus;
  assign out_single = in_single;
endmodule

module top (input [1:0] top_in_bus, input top_in_single,
            output [2:0] top_out_bus, output top_out_single);
  wire [1:0] sub_out_bus;
  wire       sub_out_single;
  sub_module sub_inst (.in_bus(top_in_bus), .in_single(top_in_single),
                       .out_bus(sub_out_bus), .out_single(sub_out_single));
  assign top_out_bus = {top_in_bus[0], sub_out_bus[1], sub_out_single};
  assign top_out_single = sub_out_bus[0];
endmodule

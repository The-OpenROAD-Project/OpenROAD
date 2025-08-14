// get_ports1.v

module sub_module (
  input [3:0] in_bus,
  input       in_single,
  output [1:0] out_bus,
  output      out_single
);
  // Dummy logic
  wire in_single_buf;
  INV_X1 buf_inst (.A(in_single), .ZN(in_single_buf));
  assign out_bus = in_bus[3:2];
  assign out_single = in_single_buf;
endmodule

module top (
  input [7:0] top_in_bus,
  input       top_in_single,
  output [4:0] top_out_bus,
  output      top_out_single,
  input       clk
);

  wire [1:0] sub_out_bus;
  wire       sub_out_single;

  sub_module sub_inst (
    .in_bus(top_in_bus[3:0]),
    .in_single(top_in_single),
    .out_bus(sub_out_bus),
    .out_single(sub_out_single)
  );

  assign top_out_bus = {top_in_bus[7:5], sub_out_bus[1], sub_out_single};
  assign top_out_single = sub_out_bus[0];

endmodule
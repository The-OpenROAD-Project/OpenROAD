// Non-constant `inside` pattern: the frontend cannot fold the item, and
// raises an unimplemented-expression diagnostic naming the item.
module top (input wire [3:0] a, input wire [3:0] b, output wire z);
  assign z = a inside {b};
endmodule

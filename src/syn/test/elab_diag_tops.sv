// Two top-level modules and no --top: the driver requires exactly one.
module a (input wire x, output wire y);
  assign y = x;
endmodule

module b (input wire x, output wire y);
  assign y = ~x;
endmodule

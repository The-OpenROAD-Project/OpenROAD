// Elaborated with --keep-hierarchy, which the Yosys-free build does not
// support. The diagnostic comes from the vendored frontend's log_error().
module sub (input wire x, output wire y);
  assign y = ~x;
endmodule

module top (input wire x, output wire y);
  sub u (.x(x), .y(y));
endmodule

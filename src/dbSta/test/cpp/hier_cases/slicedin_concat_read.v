// TOP: top
// TECH: nangate45
// TARGETS: sliced_instance_input, concat_read, bus_slice_assign
// CLUE: T1 and T2 together in minimal form: narrowing-slice instance input AND a top-level concat read of the sub's feedthrough output wire.

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [7:0] i, output [1:0] o);
  wire [1:0] m;
  sub u0 (.a(i[3:0]), .y(m));
  assign o = {m[0], m[1]};
endmodule

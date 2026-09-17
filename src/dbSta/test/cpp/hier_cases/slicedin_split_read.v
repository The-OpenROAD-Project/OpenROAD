// TOP: top
// TECH: nangate45
// TARGETS: sliced_instance_input, split_bit_read, bus_slice_assign
// CLUE: suspect T1 minimal: narrowing-slice instance input plus the sub's feedthrough bus output read bit-wise into two different scalar top outputs.

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [7:0] i, output o1, output o2);
  wire [1:0] m;
  sub u0 (.a(i[3:0]), .y(m));
  assign o1 = m[0];
  assign o2 = m[1];
endmodule

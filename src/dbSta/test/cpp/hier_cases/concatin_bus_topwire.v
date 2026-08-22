// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, concat_instance_input, internal_top_wire
// CLUE: instance bus input given as a full-width CONCAT of top bits (same width, still needs expansion): separates 'connection needs expansion' from 'narrowing slice'.

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [3:0] i, input zi, output [1:0] o, output zo);
  wire [1:0] m;
  sub u0 (.a({i[3], i[2], i[1], i[0]}), .y(m));
  assign o = m;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule

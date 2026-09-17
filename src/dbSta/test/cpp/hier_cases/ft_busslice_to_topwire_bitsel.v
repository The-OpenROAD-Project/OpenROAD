// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, internal_top_wire, bit_select_read
// CLUE: sub bus-slice feedthrough onto an internal top wire whose BITS are read individually into scalar top outputs (the top_out_single = sub_out_bus[0] shape).

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [3:0] i, input zi, output o1, output o2, output zo);
  wire [1:0] m;
  sub u0 (.a(i), .y(m));
  assign o1 = m[0];
  assign o2 = m[1];
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule

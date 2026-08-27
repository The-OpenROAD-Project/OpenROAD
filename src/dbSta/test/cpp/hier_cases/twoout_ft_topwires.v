// TOP: top
// TECH: nangate45
// TARGETS: two_feedthrough_outputs, whole_connections, internal_top_wires
// CLUE: sub with TWO feedthrough outputs (one bus slice, one scalar) but every instance connection is whole-port: isolates multi-output feedthrough from input slicing.

module sub (input [3:0] a, input b, output [1:0] y, output z);
  assign y = a[3:2];
  assign z = b;
endmodule

module top (input [3:0] i, input j, output o1, output o2, output o3);
  wire [1:0] m;
  wire n;
  sub u0 (.a(i), .b(j), .y(m), .z(n));
  assign o1 = m[0];
  assign o2 = m[1];
  assign o3 = n;
endmodule

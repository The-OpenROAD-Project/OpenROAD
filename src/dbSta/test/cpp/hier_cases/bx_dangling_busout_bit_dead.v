// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_output_bit_dangling
// CLUE: sub drives all 4 bits of output bus q; parent consumes only q[3:1]
//       (via assign to output bus y), q[0] dangles in wire qw[0].
module sub (a, q);
  input a;
  output [3:0] q;
  INV_X1 g0 (.A(a), .ZN(q[0]));
  BUF_X1 g1 (.A(a), .Z(q[1]));
  INV_X1 g2 (.A(a), .ZN(q[2]));
  BUF_X1 g3 (.A(a), .Z(q[3]));
endmodule

module top (x, y);
  input x;
  output [2:0] y;
  wire [3:0] qw;
  sub u0 (.a(x), .q(qw));
  assign y = qw[3:1];
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: ascending_range, top_port_0to3, whole_bus_connect
// CLUE: Top-level ports ascending [0:3] feeding a descending [3:0] child
// whole-bus. The emitted top port range direction must survive the trip.
module sub (p, q);
  input [3:0] p;
  output [3:0] q;
  INV_X1 g0 (.A(p[0]), .ZN(q[0]));
  BUF_X1 g1 (.A(p[1]), .Z(q[1]));
  NAND2_X1 g2 (.A1(p[2]), .A2(p[3]), .ZN(q[2]));
  XOR2_X1 g3 (.A(p[3]), .B(p[0]), .Z(q[3]));
endmodule
module top (in, out);
  input [0:3] in;
  output [0:3] out;
  sub u0 (.p(in), .q(out));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: dangling_bus_bit, concat_conn, dead_wire
// CLUE: one bit of a bus connection is a declared-but-undriven wire deadw feeding only
// a dead cone inside sub: .b({deadw, in3, in2, in1}). LEC proved; deadw fate?
module sub (input [3:0] b, output y);
  wire t01;
  wire d3;
  XOR2_X1 g1 (.A(b[0]), .B(b[1]), .Z(t01));
  XOR2_X1 g2 (.A(t01), .B(b[2]), .Z(y));
  INV_X1 g3 (.A(b[3]), .ZN(d3));
endmodule
module top (input in1, input in2, input in3, output out1);
  wire deadw;
  sub u1 (.b({deadw, in3, in2, in1}), .y(out1));
endmodule

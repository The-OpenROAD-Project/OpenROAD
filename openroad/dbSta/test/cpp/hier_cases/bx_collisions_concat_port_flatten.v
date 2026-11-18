// TOP: top
// TECH: nangate45
// TARGETS: synthesized_name_probe, concat_port
// CLUE: bus port of x fed by a concat at top; probes what net names the
// flattener synthesizes for the port bits (candidates for future escaped
// collisions).
module sub2b (input [1:0] a, output z);
  XOR2_X1 g (.A(a[0]), .B(a[1]), .Z(z));
endmodule

module top (input in1, input in2, output o1);
  sub2b x (.a({in2, in1}), .z(o1));
endmodule

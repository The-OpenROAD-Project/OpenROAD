// TOP: top
// TECH: nangate45
// TARGETS: assign_const_output_port, submodule, bus_const
// CLUE: submodule assigns a 4-bit constant to its output BUS port; top
// consumes all four bits — bus-wide constant crossing a boundary upward.
module sub (output [3:0] c);
  assign c = 4'b0110;
endmodule

module top (input a, output y);
  wire [3:0] t;
  wire n1, n2, n3;
  sub s1 (.c(t));
  AND2_X1 g1 (.A1(t[0]), .A2(t[1]), .ZN(n1));
  AND2_X1 g2 (.A1(t[2]), .A2(t[3]), .ZN(n2));
  OR2_X1 g3 (.A1(n1), .A2(n2), .ZN(n3));
  XOR2_X1 g4 (.A(n3), .B(a), .Z(y));
endmodule

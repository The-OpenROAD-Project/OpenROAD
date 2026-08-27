// TOP: top
// TECH: nangate45
// TARGETS: assign_const_wire, const_fanout
// CLUE: a single assign w = 1'b1 feeding THREE gate pins — one constant net
// with fanout 3; writer may duplicate the constant per sink.
module top (input a, input b, output y1, output y2, output y3);
  wire w;
  assign w = 1'b1;
  AND2_X1 g1 (.A1(a), .A2(w), .ZN(y1));
  NAND2_X1 g2 (.A1(b), .A2(w), .ZN(y2));
  XOR2_X1 g3 (.A(a), .B(w), .Z(y3));
endmodule

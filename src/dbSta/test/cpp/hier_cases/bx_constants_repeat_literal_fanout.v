// TOP: top
// TECH: nangate45
// TARGETS: tie0, literal_fanout
// CLUE: the literal 1'b0 written independently at THREE different cell pins in
// one module — reader may merge them into one constant net or keep three;
// writer must not drop any tie.
module top (input a, input b, output y1, output y2, output y3);
  OR2_X1 g1 (.A1(a), .A2(1'b0), .ZN(y1));
  NOR2_X1 g2 (.A1(b), .A2(1'b0), .ZN(y2));
  XNOR2_X1 g3 (.A(a), .B(1'b0), .ZN(y3));
endmodule

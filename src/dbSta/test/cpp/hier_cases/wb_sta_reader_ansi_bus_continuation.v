// TOP: top
// TECH: nangate45
// TARGETS: ansi_header, dcl_continuation, bus_range_inheritance
// CLUE: port_dcls ',' dcl_arg (VerilogParse.yy:191) appends the continuation arg to the
// CLUE: PREVIOUS declaration with a raw dynamic_cast, so b must inherit a's [1:0] range
// CLUE: and the ports must keep header order.
module top (input [1:0] a, b, output y);
  wire n1;
  wire n2;
  XOR2_X1 g0 (.A(a[0]), .B(b[0]), .Z(n1));
  XOR2_X1 g1 (.A(a[1]), .B(b[1]), .Z(n2));
  NAND2_X1 g2 (.A1(n1), .A2(n2), .ZN(y));
endmodule

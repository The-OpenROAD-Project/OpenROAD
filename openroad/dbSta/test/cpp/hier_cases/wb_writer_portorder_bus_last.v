// TOP: top
// TECH: nangate45
// TARGETS: port_order, bus_ports_last, groupBusPorts
// CLUE: ConcreteCell::groupBusPorts (ConcreteLibrary.cc:360-397) rebuilds ports_ with the
// CLUE: scalars in bterm order and then APPENDS the grouped buses in std::map (ASCII)
// CLUE: order, so declaration order is lost twice over: expect (ma, y, zc, abus, zbus).
module top (zc, ma, zbus, abus, y);
  input zc;
  input ma;
  input [1:0] zbus;
  input [1:0] abus;
  output y;
  wire n1;
  wire n2;
  wire n3;
  wire n4;
  XOR2_X1 g1 (.A(zc), .B(ma), .Z(n1));
  XOR2_X1 g2 (.A(zbus[0]), .B(zbus[1]), .Z(n2));
  XOR2_X1 g3 (.A(abus[0]), .B(abus[1]), .Z(n3));
  XOR2_X1 g4 (.A(n1), .B(n2), .Z(n4));
  XOR2_X1 g5 (.A(n4), .B(n3), .Z(y));
endmodule

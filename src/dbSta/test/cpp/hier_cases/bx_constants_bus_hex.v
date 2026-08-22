// TOP: top
// TECH: nangate45
// TARGETS: bus_const, hex_literal
// CLUE: whole 4-bit bus port driven by hex literal 4'hA; hex base conversion
// and nibble bit order.
module top (a, y);
  input a;
  output y;
  wire p;
  sub s (.bus(4'hA), .p(p));
  XOR2_X1 g (.A(a), .B(p), .Z(y));
endmodule

module sub (bus, p);
  input [3:0] bus;
  output p;
  wire t0, t1;
  XOR2_X1 x0 (.A(bus[0]), .B(bus[1]), .Z(t0));
  XOR2_X1 x1 (.A(bus[2]), .B(bus[3]), .Z(t1));
  XOR2_X1 x2 (.A(t0), .B(t1), .Z(p));
endmodule

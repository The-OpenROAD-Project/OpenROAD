// TOP: top
// TECH: nangate45
// TARGETS: bus_const, decimal_literal
// CLUE: whole 4-bit bus port driven by decimal literal 4'd5; decimal base
// conversion in the reader.
module top (a, y);
  input a;
  output y;
  wire p;
  sub s (.bus(4'd5), .p(p));
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

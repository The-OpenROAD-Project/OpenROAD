// TOP: top
// TECH: nangate45
// TARGETS: bus_const, all_one
// CLUE: whole 4-bit bus port driven by 4'b1111; degenerate all-one vector
// may hit a special-cased tie-high path in the writer.
module top (a, y);
  input a;
  output y;
  wire p;
  sub s (.bus(4'b1111), .p(p));
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

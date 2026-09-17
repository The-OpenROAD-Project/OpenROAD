// TOP: top
// TECH: nangate45
// TARGETS: net_decl_assignment, bus, dcl_map, width
// CLUE: VerilogModule::parseDcl (VerilogReader.cc:608) only registers dcl args that are
// CLUE: NAMED; an arg holding a net-declaration assignment is skipped, so "wire [3:0] w ="
// CLUE: leaves w with no declaration and verilogNetScalarSize() reports size 1.
module sub (d, o);
  input [3:0] d;
  output o;
  wire n1;
  wire n2;
  XOR2_X1 g1 (.A(d[0]), .B(d[1]), .Z(n1));
  XOR2_X1 g2 (.A(d[2]), .B(d[3]), .Z(n2));
  XNOR2_X1 g3 (.A(n1), .B(n2), .ZN(o));
endmodule

module top (a, b, c, e, y);
  input a;
  input b;
  input c;
  input e;
  output y;
  wire [3:0] w = {a, b, c, e};
  sub u (.d(w), .o(y));
endmodule

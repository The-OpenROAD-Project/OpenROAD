// TOP: top
// TECH: nangate45
// TARGETS: needs_escape, leading_digit, bus_base_name, netVerilogName
// CLUE: netVerilogName splits a bus subscript off and spells the BASE with staToVerilog
// CLUE: (VerilogNamespace.cc:63) -- a different call than the scalar path.  A digit-
// CLUE: leading escaped BUS name must come back as "wire [1:0] 1b;" / ".A(1b[0])", and
// CLUE: "1b" additionally lexes as the head of a sized literal.
module top (a, z);
  input [1:0] a;
  output [1:0] z;
  wire [1:0] \1b ;
  INV_X1 g0 (.A(a[0]), .ZN(\1b [0]));
  INV_X1 g1 (.A(a[1]), .ZN(\1b [1]));
  BUF_X1 h0 (.A(\1b [0]), .Z(z[0]));
  BUF_X1 h1 (.A(\1b [1]), .Z(z[1]));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: redeclaration, bus, dcl_map_replace
// CLUE: parseDcl's redeclaration branch (VerilogReader.cc:612-637) replaces an existing
// CLUE: INTERNAL dcl with a later port dcl.  A bus wire dcl is not pruned, so
// CLUE: "wire [1:0] y;" before "output [1:0] y;" is the only way to reach that branch.
module top (a, y);
  wire [1:0] y;
  input [1:0] a;
  output [1:0] y;
  INV_X1 g0 (.A(a[0]), .ZN(y[0]));
  BUF_X1 g1 (.A(a[1]), .Z(y[1]));
endmodule

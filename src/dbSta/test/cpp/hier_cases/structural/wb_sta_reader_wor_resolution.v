// TOP: top
// TECH: nangate45
// TARGETS: wor, net_type, resolution_function, multi_driver
// CLUE: dcl_type maps wand/wor to PortDirection::internal() (VerilogParse.yy:336-338) so
// CLUE: the resolution function is dropped, and makeDcl prunes the scalar dcl entirely.
// CLUE: The two continuous assigns then MERGE a and b into one net (mergeAssignNet),
// CLUE: shorting two primary inputs instead of OR-ing them.
module top (a, b, y);
  input a;
  input b;
  output y;
  wor w;
  assign w = a;
  assign w = b;
  BUF_X1 g (.A(w), .Z(y));
endmodule

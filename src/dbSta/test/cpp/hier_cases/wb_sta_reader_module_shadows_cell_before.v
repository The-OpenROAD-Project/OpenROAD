// TOP: top
// TECH: nangate45
// TARGETS: cell_name_collision, findAnyCell, module_order
// CLUE: makeModuleInst resolves the master with findAnyCell AT PARSE TIME
// CLUE: (VerilogReader.cc:438).  Here the user module BUF_X1 is defined BEFORE the
// CLUE: instantiation, so the verilog library hit should win and y must be ~a.
module BUF_X1 (A, Z);
  input A;
  output Z;
  INV_X1 g (.A(A), .ZN(Z));
endmodule

module top (a, y);
  input a;
  output y;
  BUF_X1 u (.A(a), .Z(y));
endmodule

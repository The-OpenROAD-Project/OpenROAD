// TOP: top
// TECH: nangate45
// TARGETS: cell_name_collision, findAnyCell, module_order
// CLUE: same design with the user module BUF_X1 defined AFTER the instantiation.  At
// CLUE: parse time findAnyCell only sees the liberty BUF_X1, so hasScalarNamedPortRefs
// CLUE: fires and the instance is frozen into a VerilogLibertyInst -> the user module is
// CLUE: never elaborated and y becomes a, not ~a.  Declaration order changes the logic.
module top (a, y);
  input a;
  output y;
  BUF_X1 u (.A(a), .Z(y));
endmodule

module BUF_X1 (A, Z);
  input A;
  output Z;
  INV_X1 g (.A(A), .ZN(Z));
endmodule

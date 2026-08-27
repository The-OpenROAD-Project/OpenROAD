// TOP: top
// TECH: nangate45
// TARGETS: dead_wire_declaration
// CLUE: wire dead is declared and never referenced by anything. The most
//       minimal dangling object; does the declaration survive?
module top (x, y);
  input x;
  output y;
  wire dead;
  INV_X1 u1 (.A(x), .ZN(y));
endmodule

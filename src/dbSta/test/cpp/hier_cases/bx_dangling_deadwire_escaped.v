// TOP: top
// TECH: nangate45
// TARGETS: dead_wire_declaration, escaped_name
// CLUE: escaped-name wire \dead! declared and never referenced. Dangling +
//       escaping; if kept, escaping must be kept too.
module top (x, y);
  input x;
  output y;
  wire \dead! ;
  INV_X1 u1 (.A(x), .ZN(y));
endmodule

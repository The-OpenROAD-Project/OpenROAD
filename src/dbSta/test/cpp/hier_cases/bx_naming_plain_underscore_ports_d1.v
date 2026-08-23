// TOP: top
// TECH: nangate45
// TARGETS: leading_underscore, port, depth_1
// CLUE: Leading-underscore top-level port names (_a, __b, _y).
module top (_a, __b, _y);
  input _a, __b;
  output _y;
  wire n;
  AND2_X1 u1 (.A1(_a), .A2(__b), .ZN(n));
  INV_X1 u2 (.A(n), .ZN(_y));
endmodule

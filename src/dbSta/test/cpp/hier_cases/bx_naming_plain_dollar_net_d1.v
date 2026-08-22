// TOP: top
// TECH: nangate45
// TARGETS: dollar_name, net, depth_1
// CLUE: $ in plain net names (a$b, x$, a$$b, a$b$c) at top level.
module top (a, b, y);
  input a, b;
  output y;
  wire a$b;
  wire x$;
  wire a$$b;
  wire a$b$c;
  AND2_X1 u1 (.A1(a), .A2(b), .ZN(a$b));
  INV_X1 u2 (.A(a$b), .ZN(x$));
  BUF_X1 u3 (.A(x$), .Z(a$$b));
  XOR2_X1 u4 (.A(a$$b), .B(a), .Z(a$b$c));
  INV_X1 u5 (.A(a$b$c), .ZN(y));
endmodule

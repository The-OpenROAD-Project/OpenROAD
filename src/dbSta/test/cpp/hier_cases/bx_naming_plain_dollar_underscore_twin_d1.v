// TOP: top
// TECH: nangate45
// TARGETS: dollar_name, escape_collision, net, instance, depth_1
// CLUE: net a$b next to lookalike net a_b, instance u$1 next to u_1; a
// writer that maps $ to _ instead of escaping would collide the pairs.
module top (p, q, y);
  input p, q;
  output y;
  wire a$b;
  wire a_b;
  wire n1;
  NAND2_X1 u$1 (.A1(p), .A2(q), .ZN(a$b));
  NOR2_X1 u_1 (.A1(p), .A2(q), .ZN(a_b));
  XOR2_X1 u2 (.A(a$b), .B(a_b), .Z(n1));
  INV_X1 u3 (.A(n1), .ZN(y));
endmodule

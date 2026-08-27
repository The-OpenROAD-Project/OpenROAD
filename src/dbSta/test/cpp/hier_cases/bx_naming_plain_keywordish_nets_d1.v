// TOP: top
// TECH: nangate45
// TARGETS: keyword_adjacent, net, instance, depth_1
// CLUE: Legal names one character away from keywords: nets module1, wire_, assign2, endmodule2; instances always1, reg_.
module top (a, b, y);
  input a, b;
  output y;
  wire module1, wire_, assign2, endmodule2;
  AND2_X1 always1 (.A1(a), .A2(b), .ZN(module1));
  INV_X1 reg_ (.A(module1), .ZN(wire_));
  BUF_X1 u3 (.A(wire_), .Z(assign2));
  XOR2_X1 u4 (.A(assign2), .B(a), .Z(endmodule2));
  INV_X1 u5 (.A(endmodule2), .ZN(y));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: case_sensitivity, net, depth_1
// CLUE: data, DATA, Data as three DISTINCT nets in one scope; case-folding would alias them.
module top (a, b, y);
  input a, b;
  output y;
  wire data, DATA, Data;
  AND2_X1 u1 (.A1(a), .A2(b), .ZN(data));
  OR2_X1 u2 (.A1(a), .A2(b), .ZN(DATA));
  XOR2_X1 u3 (.A(data), .B(DATA), .Z(Data));
  INV_X1 u4 (.A(Data), .ZN(y));
endmodule

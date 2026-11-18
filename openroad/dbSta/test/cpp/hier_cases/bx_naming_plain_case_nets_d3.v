// TOP: top
// TECH: nangate45
// TARGETS: case_sensitivity, net, depth_3
// CLUE: data/DATA/Data distinct nets inside the leaf of a depth-3 hierarchy; flat writer prefixes them with the same instance path.
module leaf (i, j, o);
  input i, j;
  output o;
  wire data, DATA, Data;
  AND2_X1 g1 (.A1(i), .A2(j), .ZN(data));
  OR2_X1 g2 (.A1(i), .A2(j), .ZN(DATA));
  XOR2_X1 g3 (.A(data), .B(DATA), .Z(Data));
  INV_X1 g4 (.A(Data), .ZN(o));
endmodule
module mid (i, j, o);
  input i, j;
  output o;
  wire w;
  leaf l1 (.i(i), .j(j), .o(w));
  BUF_X1 b1 (.A(w), .Z(o));
endmodule
module top (a, b, y);
  input a, b;
  output y;
  wire w;
  mid m1 (.i(a), .j(b), .o(w));
  INV_X1 u1 (.A(w), .ZN(y));
endmodule

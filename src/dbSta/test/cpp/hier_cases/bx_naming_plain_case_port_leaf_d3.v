// TOP: top
// TECH: nangate45
// TARGETS: case_sensitivity, port, depth_3
// CLUE: leaf ports p and P differ only by case with asymmetric functions;
// case-collapse at a submodule boundary misconnects and is LEC-visible.
module leaf (p, P, o, O);
  input p, P;
  output o, O;
  INV_X1 g1 (.A(p), .ZN(o));
  BUF_X1 g2 (.A(P), .Z(O));
endmodule
module mid (i, j, x, X);
  input i, j;
  output x, X;
  leaf l1 (.p(i), .P(j), .o(x), .O(X));
endmodule
module top (a, b, y1, y2);
  input a, b;
  output y1, y2;
  wire w1, w2;
  mid m1 (.i(a), .j(b), .x(w1), .X(w2));
  INV_X1 u1 (.A(w1), .ZN(y1));
  BUF_X1 u2 (.A(w2), .Z(y2));
endmodule

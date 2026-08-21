// TOP: top
// TECH: nangate45
// TARGETS: case_sensitivity, instance, depth_3, path_collision
// CLUE: Sibling module instances a and A (case-only difference); flattened paths a/b/g vs A/b/g must stay distinct.
module leaf (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module mid (i, o);
  input i;
  output o;
  leaf b (.i(i), .o(o));
endmodule
module top (x1, x2, y1, y2);
  input x1, x2;
  output y1, y2;
  wire w1;
  mid a (.i(x1), .o(y1));
  mid A (.i(x2), .o(w1));
  BUF_X1 u1 (.A(w1), .Z(y2));
endmodule

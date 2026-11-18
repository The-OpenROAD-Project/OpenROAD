// TOP: top
// TECH: nangate45
// TARGETS: name_interplay, top_like_module, depth_1
// CLUE: Submodule named top1 (one digit away from the top name).
module top1 (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  top1 u1 (.i(a), .o(w));
  INV_X1 u2 (.A(w), .ZN(y));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: name_interplay, top_like_module, depth_1
// CLUE: Submodule named top_ (one underscore away from the top name).
module top_ (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  top_ u1 (.i(a), .o(w));
  INV_X1 u2 (.A(w), .ZN(y));
endmodule

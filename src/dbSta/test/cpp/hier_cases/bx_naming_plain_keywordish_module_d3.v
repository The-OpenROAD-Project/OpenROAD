// TOP: top
// TECH: nangate45
// TARGETS: keyword_adjacent, module_name, depth_3
// CLUE: Keyword-adjacent module names module1 and wire_ in a depth-3 chain.
module wire_ (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module module1 (i, o);
  input i;
  output o;
  wire w;
  wire_ u (.i(i), .o(w));
  BUF_X1 b (.A(w), .Z(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  module1 u (.i(a), .o(w));
  INV_X1 v (.A(w), .ZN(y));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, trailing_char_pair, depth_1
// CLUE: two escaped nets \sig% and \sig%% differing only by one trailing char; truncation would merge them
module top (a, b, y, z);
  input a, b;
  output y, z;
  wire \sig% ;
  wire \sig%% ;
  AND2_X1 g0 (.A1(a), .A2(b), .ZN(\sig% ));
  OR2_X1 g1 (.A1(a), .A2(b), .ZN(\sig%% ));
  BUF_X1 g2 (.A(\sig% ), .Z(y));
  BUF_X1 g3 (.A(\sig%% ), .Z(z));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, char_plus, depth_3
// CLUE: escaped instance \i+3 inside a depth-3 leaf; flat path name mid/leaf/i+3 needs escaping
module top (a, z);
  input a;
  output z;
  mid m1 (.i(a), .o(z));
endmodule
module mid (i, o);
  input i;
  output o;
  leaf l1 (.i(i), .o(o));
endmodule
module leaf (i, o);
  input i;
  output o;
  wire n1;
  BUF_X1 \i+3 (.A(i), .Z(n1));
  INV_X1 b2 (.A(n1), .ZN(o));
endmodule

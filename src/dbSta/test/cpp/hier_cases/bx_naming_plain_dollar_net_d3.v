// TOP: top
// TECH: nangate45
// TARGETS: dollar_name, net, depth_3
// CLUE: $ in plain net names inside every level of a depth-3 hierarchy.
module leaf (i, o);
  input i;
  output o;
  wire n$1;
  wire n$$;
  INV_X1 g1 (.A(i), .ZN(n$1));
  BUF_X1 g2 (.A(n$1), .Z(n$$));
  INV_X1 g3 (.A(n$$), .ZN(o));
endmodule
module mid (i, o);
  input i;
  output o;
  wire w$m;
  leaf l1 (.i(i), .o(w$m));
  BUF_X1 b1 (.A(w$m), .Z(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire t$w;
  mid m1 (.i(a), .o(t$w));
  INV_X1 u1 (.A(t$w), .ZN(y));
endmodule

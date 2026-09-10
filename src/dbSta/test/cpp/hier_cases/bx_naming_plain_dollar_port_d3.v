// TOP: top
// TECH: nangate45
// TARGETS: dollar_name, port, depth_3
// CLUE: $ in submodule port names at depth 3; hier writer must re-emit them, flat writer must resolve connections through them.
module leaf (p$i, q$o);
  input p$i;
  output q$o;
  INV_X1 g (.A(p$i), .ZN(q$o));
endmodule
module mid (m$i, m$o);
  input m$i;
  output m$o;
  wire w;
  leaf l1 (.p$i(m$i), .q$o(w));
  BUF_X1 b1 (.A(w), .Z(m$o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  mid m1 (.m$i(a), .m$o(w));
  INV_X1 u1 (.A(w), .ZN(y));
endmodule

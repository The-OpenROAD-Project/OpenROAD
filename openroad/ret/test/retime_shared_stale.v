// A net with two drivers, da and db, next to a slide.  The slide of ffa
// across `big` puts the pin of `big` on m: nd slows, and so does da through
// the slower edge at its input.  db is the deeper driver, the one STA times
// the net from.
module top (a, b, s, clk, y, po);
  input a;
  input b;
  input s;
  input clk;
  output y;
  output po;
  wire d0, q0, m, qa, shared, qb;
  wire w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12;
  wire s0, s1, s2;

  AND2_X1 gin (.A1(a), .A2(b), .ZN(d0));
  DFF_X1 ff0 (.D(d0), .CK(clk), .Q(q0));
  NAND4_X1 nd (.A1(q0), .A2(q0), .A3(q0), .A4(q0), .ZN(m));
  DFF_X1 ffa (.D(m), .CK(clk), .Q(qa));
  INV_X4 big (.A(qa), .ZN(w0));
  INV_X1 ic1 (.A(w0), .ZN(w1));
  INV_X1 ic2 (.A(w1), .ZN(w2));
  INV_X1 ic3 (.A(w2), .ZN(w3));
  INV_X1 ic4 (.A(w3), .ZN(w4));
  INV_X1 ic5 (.A(w4), .ZN(w5));
  INV_X1 ic6 (.A(w5), .ZN(w6));
  INV_X1 ic7 (.A(w6), .ZN(w7));
  INV_X1 ic8 (.A(w7), .ZN(w8));
  INV_X1 ic9 (.A(w8), .ZN(w9));
  INV_X1 ic10 (.A(w9), .ZN(w10));
  INV_X1 ic11 (.A(w10), .ZN(w11));
  INV_X1 ic12 (.A(w11), .ZN(w12));
  DFF_X1 ffb (.D(w12), .CK(clk), .Q(qb));
  BUF_X1 gout (.A(qb), .Z(y));
  INV_X1 da (.A(m), .ZN(shared));
  BUF_X1 sb0 (.A(s), .Z(s0));
  BUF_X1 sb1 (.A(s0), .Z(s1));
  BUF_X1 sb2 (.A(s1), .Z(s2));
  INV_X1 db (.A(s2), .ZN(shared));
  INV_X1 oi (.A(shared), .ZN(po));
endmodule

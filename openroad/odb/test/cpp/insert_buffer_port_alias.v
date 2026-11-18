module top (input a, output p, q, y);
  INV_X1 drv (.A(a), .ZN(p));
  assign q = p;
  INV_X1 ld (.A(p), .ZN(y));
endmodule

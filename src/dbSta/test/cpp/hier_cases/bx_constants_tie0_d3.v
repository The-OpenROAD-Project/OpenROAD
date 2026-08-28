// TOP: top
// TECH: nangate45
// TARGETS: tie_low, depth_3
// CLUE: literal 1'b0 tie two module levels below top; deep constant nets
// stress flat uniquification and hier module re-emission.
module top (a, y);
  input a;
  output y;
  mid m1 (.i(a), .o(y));
endmodule

module mid (i, o);
  input i;
  output o;
  leaf l1 (.i(i), .o(o));
endmodule

module leaf (i, o);
  input i;
  output o;
  wire w0;
  OR2_X1 u1 (.A1(i), .A2(1'b0), .ZN(w0));
  INV_X1 u2 (.A(w0), .ZN(o));
endmodule

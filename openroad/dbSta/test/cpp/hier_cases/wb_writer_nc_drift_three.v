// TOP: top
// TECH: nangate45
// TARGETS: nc_filler, hier, counter_drift, undeclared_net
// CLUE: unconnected_net_index_ is a VerilogWriter member that is NEVER reset per module
// CLUE: (VerilogWriter.cc:88,430) while writeWireDcls always declares _NC1.._NCn from 1
// CLUE: (VerilogWriter.cc:310-312).  With three parent modules the drift accumulates:
// CLUE: m1 must reference _NC3/_NC4 and m2 _NC5/_NC6 while both declare only _NC1/_NC2.
module leaf (s, i, o);
  input s;
  input [1:0] i;
  output o;
  INV_X1 g (.A(s), .ZN(o));
endmodule

module m1 (a, y);
  input a;
  output y;
  leaf L (.s(a), .o(y));
endmodule

module m2 (a, y);
  input a;
  output y;
  leaf L (.s(a), .o(y));
endmodule

module top (a, b, c, y1, y2, y3);
  input a;
  input b;
  input c;
  output y1;
  output y2;
  output y3;
  leaf LT (.s(a), .o(y1));
  m1 U1 (.a(b), .y(y2));
  m2 U2 (.a(c), .y(y3));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: nc_filler, inout_formal, vector_formal
// CLUE: findPortNCcount never looks at PortDirection, so an unconnected INOUT vector
// CLUE: formal also gets _NC fillers; verilogPortDir maps bidirect to "inout".
module leaf (s, b, o);
  input s;
  inout [1:0] b;
  output o;
  INV_X1 g (.A(s), .ZN(o));
endmodule

module top (a, y);
  input a;
  output y;
  leaf L (.s(a), .o(y));
endmodule

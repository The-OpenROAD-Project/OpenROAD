// TOP: top
// TECH: nangate45
// TARGETS: unconnected_formal, hasTerminals_skip, internal_fanout
// CLUE: Verilog2db::makeDbNets skips every non-top net that hasTerminals()
// (dbReadVerilog.cc:708) on the assumption the parent-side net will carry the
// connectivity. When the formal is left OPEN at the instantiation there is no
// parent net, so the child net t -- which also feeds an internal load -- gets
// no dbNet at all and the internal cone should lose its driver in flat mode.
module top (a, y);
   input a;
   output y;
   sub u (.i(a), .o(y), .t());
endmodule

module sub (i, o, t);
   input i;
   output o;
   output t;
   INV_X1 g0 (.A(i), .ZN(t));
   BUF_X1 g1 (.A(t), .Z(o));
endmodule

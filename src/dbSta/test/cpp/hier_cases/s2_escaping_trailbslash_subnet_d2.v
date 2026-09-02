// TARGETS: escaped_net, trailing_backslash, depth_2
// CLUE: verilogToSta (VerilogNamespace.cc:205-215) DOUBLES a trailing backslash,
// so `\w\ ` becomes sta "w\\". As a module-local net at depth 2 the flat dbNet
// name is "u1/w\\": the backslash run sits AFTER the divider, so
// dbNetwork::stripParentPrefix (dbNetwork.cc:1398) and dbBlock::getBaseName
// (dbBlock.cpp:3969) must both still split at index 2. Corpus covers the
// trailing backslash only on an instance name at the top; this is the net kind.
module top (a, y);
   input a;
   output y;
   m u1 (.i(a), .o(y));
endmodule

module m (i, o);
   input i;
   output o;
   wire \w\ ;
   INV_X1 g1 (.A(i), .ZN(\w\ ));
   BUF_X1 g2 (.A(\w\ ), .Z(o));
endmodule

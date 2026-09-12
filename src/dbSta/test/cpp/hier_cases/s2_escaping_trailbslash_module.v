// TARGETS: escaped_module, trailing_backslash, depth_2
// CLUE: module name `\m\ ` -> sta "m\\". makeUniqueDbModule (dbReadVerilog.cc:376)
// stores it as the dbModule name and dbModule::getHierarchicalName /
// dbBlock::getBaseName (dbBlock.cpp:3969) parse it with backslash-run parity,
// while staToVerilog (VerilogNamespace.cc:83) collapses "\\" back to one
// backslash on the way out. Corpus has no trailing-backslash module name.
module top (a, y);
   input a;
   output y;
   \m\  u1 (.i(a), .o(y));
endmodule

module \m\  (i, o);
   input i;
   output o;
   INV_X1 g1 (.A(i), .ZN(o));
endmodule

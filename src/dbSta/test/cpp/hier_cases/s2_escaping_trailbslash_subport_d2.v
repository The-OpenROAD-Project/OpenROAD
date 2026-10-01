// TARGETS: escaped_port, trailing_backslash, depth_2
// CLUE: submodule port `\i\ ` -> sta "i\\". The name lands on a dbModBTerm and a
// dbModITerm (dbReadVerilog.cc:485,514) and is looked up through
// dbModule::findModBTerm -> dbBlock::getBaseName (dbModule.cpp:568,
// dbBlock.cpp:3969), whose backslash-run parity differs from
// dbNetwork::stripParentPrefix (dbNetwork.cc:1398/1434). The writer must emit
// both the declaration and the `.\i\ (a)` connection with the escape intact.
module top (a, y);
   input a;
   output y;
   m u1 (.\i\ (a), .o(y));
endmodule

module m (\i\ , o);
   input \i\ ;
   output o;
   INV_X1 g1 (.A(\i\ ), .ZN(o));
endmodule

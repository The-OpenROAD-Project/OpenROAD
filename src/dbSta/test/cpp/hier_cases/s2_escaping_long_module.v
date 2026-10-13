// TARGETS: escaped_module, very_long_name, depth_2
// CLUE: a 300-character escaped module name. dbModule::makeUniqueDbModule
// (dbReadVerilog.cc:376) keys the module table on this string and appends a
// uniquification suffix to it; dbBlock::getBaseName (dbBlock.cpp:3969) walks it
// char by char. Nothing in the corpus exercises a name longer than 26 chars.
module top (input a, output z);
  \m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-  u1 (.a(a), .z(z));
endmodule
module \m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-m-  (input a, output z);
  INV_X1 g1 (.A(a), .ZN(z));
endmodule

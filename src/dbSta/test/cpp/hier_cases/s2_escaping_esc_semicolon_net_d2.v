// TARGETS: escaped_net, char_semicolon, depth_2
// CLUE: ';' is the statement terminator, so losing the escape does not merely
// rename the net -- it truncates the declaration and the rest of the module
// becomes garbage. Exactly one corpus file uses ';' in an escaped name and only
// as a top-level net. Here it is a module-local net one level down, so the flat
// join in dbReadVerilog.cc:538 makes "u1/w;x" and the hier writer must recover
// "w;x" through stripParentPrefix (dbNetwork.cc:1398) and re-escape it.
module sub (i, o);
  input i;
  output o;
  wire \w;x ;
  INV_X1 g1 (.A(i), .ZN(\w;x ));
  BUF_X1 g2 (.A(\w;x ), .Z(o));
endmodule

module top (a, z);
  input a;
  output z;
  sub u1 (.i(a), .o(z));
endmodule

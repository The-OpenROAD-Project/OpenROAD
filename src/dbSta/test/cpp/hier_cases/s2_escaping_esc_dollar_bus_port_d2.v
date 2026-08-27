// TARGETS: escaped_port, dollar_name, bus, depth_2
// CLUE: '$' is legal unescaped only after the first character, so `\$b ` needs
// the escape but `b$1` does not -- and staToVerilog2 treats '$' as
// NOT-alnum-underscore (VerilogNamespace.cc:76-80,148), so it escapes both.
// The corpus's dollar cases are all scalars; this is the bus-port kind at a
// boundary, where dbReadVerilog.cc:470 synthesizes the bit modbterm names by
// appending "[i]" to the escaped base name.
module sub (\$b , o);
  input [1:0] \$b ;
  output o;
  XOR2_X1 g1 (.A(\$b [0]), .B(\$b [1]), .Z(o));
endmodule

module top (a, z);
  input [1:0] a;
  output z;
  sub u1 (.\$b (a), .o(z));
endmodule

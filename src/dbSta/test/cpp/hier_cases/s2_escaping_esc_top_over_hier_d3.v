// TARGETS: escaped_module, top_module, char_star, depth_3
// CLUE: the corpus tests an escaped TOP module name only on a flat, depth-1
// design (bx_naming_escaped_top_special.v). Here the escaped top has two levels
// of hierarchy beneath it, so dbBlock::create is named from it
// (dbReadVerilog.cc:262), dbNetwork::name(Instance) returns it for the top
// instance via block_->getConstName() (dbNetwork.cc:1468) WITHOUT going through
// stripParentPrefix, and pathName(Net) must NOT prefix top-module nets with it
// (dbNetwork.cc:2556). A top name that needs escaping is the case where a
// stray prefix would be visible.
module leaf3 (input a, output z);
  wire \w*w ;
  INV_X1 g1 (.A(a), .ZN(\w*w ));
  BUF_X1 g2 (.A(\w*w ), .Z(z));
endmodule
module mid3 (input a, output z);
  leaf3 u2 (.a(a), .z(z));
endmodule
module \t*p (input a, output z);
  mid3 u1 (.a(a), .z(z));
endmodule

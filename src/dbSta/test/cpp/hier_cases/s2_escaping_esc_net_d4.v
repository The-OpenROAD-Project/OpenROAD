// TARGETS: escaped_net, char_symbols, depth_4
// CLUE: corpus escaped-name coverage stops at depth 3. At depth 4 the flat
// dbNet/dbInst name built by Network::pathName (Network.cc:255-268) and
// dbReadVerilog.cc:538 is u1/u2/u3/<esc>, and dbNetwork::name(Net)
// (dbNetwork.cc:2605-2634) must peel exactly THREE path components off it via
// the unanchored find/erase. One extra level is one more chance to mis-peel.
module leaf4 (input a, output z);
  wire \w-w ;
  INV_X1 g1 (.A(a), .ZN(\w-w ));
  BUF_X1 g2 (.A(\w-w ), .Z(z));
endmodule
module mid4 (input a, output z);
  leaf4 u3 (.a(a), .z(z));
endmodule
module mid3 (input a, output z);
  mid4 u2 (.a(a), .z(z));
endmodule
module top (input a, output z);
  mid3 u1 (.a(a), .z(z));
endmodule

// TARGETS: escaped_instance, very_long_name, depth_2
// CLUE: a 300-character escaped LEAF instance name inside a submodule. The flat
// dbInst name from dbReadVerilog.cc:538 is "u1/" + 300 chars, and
// dbNetwork::stripParentPrefix (dbNetwork.cc:1398) scans it from the right one
// character at a time to recover the local name for the hier writer.
module sub (input a, output z);
  INV_X1 \g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#g#  (.A(a), .ZN(z));
endmodule
module top (input a, output z);
  sub u1 (.a(a), .z(z));
endmodule

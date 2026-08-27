// TARGETS: escaped_instance, char_plus, depth_4
// CLUE: escaped LEAF instance name at depth 4. dbReadVerilog.cc:538 stores the
// dbInst under the flat path "u1/u2/u3/g+1"; dbNetwork::name(Instance)
// (dbNetwork.cc:1486) then hands that to stripParentPrefix (dbNetwork.cc:1398)
// to recover "g+1" for the hier writer. Corpus tops out at depth 3 for escaped
// instance names (bx_naming_escaped_depth3_inst.v).
module leaf4 (input a, output z);
  INV_X1 \g+1  (.A(a), .ZN(z));
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

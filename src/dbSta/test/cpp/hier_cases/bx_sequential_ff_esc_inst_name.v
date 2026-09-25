// TOP: top
// TECH: nangate45
// TARGETS: escaped_flop_instance_names, escaped_net_name, dffr_chain_2
// CLUE: the two chained flops and the net between them all carry escaped
// identifiers containing brackets and a dot -- exactly the characters the
// flattener uses when it synthesises hierarchical names.

module sub (input d, input ck, input rn, output q);
  wire \n[0] ;
  DFFR_X1 \ff[0]  (.D(d), .RN(rn), .CK(ck), .Q(\n[0] ));
  DFFR_X1 \ff.1  (.D(\n[0] ), .RN(rn), .CK(ck), .Q(q));
endmodule

module top (input d, input ck, input rn, output q);
  sub u (.d(d), .ck(ck), .rn(rn), .q(q));
endmodule

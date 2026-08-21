// TARGETS: escaped_port, char_symbols, depth_4
// CLUE: escaped ports \i^n  / \o&t  threaded through THREE boundaries. Each
// boundary makes a dbModBTerm and a dbModITerm from the same string
// (dbReadVerilog.cc:485,514) and dbNetwork::name(Port) runs it through
// stripParentPrefix (dbNetwork.cc:1434). '^' and '&' are near-absent from the
// corpus and depth 4 is uncovered for escaped ports.
module leaf4 (input \i^n , output \o&t );
  INV_X1 g1 (.A(\i^n ), .ZN(\o&t ));
endmodule
module mid4 (input \i^n , output \o&t );
  leaf4 u3 (.\i^n (\i^n ), .\o&t (\o&t ));
endmodule
module mid3 (input \i^n , output \o&t );
  mid4 u2 (.\i^n (\i^n ), .\o&t (\o&t ));
endmodule
module top (input a, output z);
  mid3 u1 (.\i^n (a), .\o&t (z));
endmodule

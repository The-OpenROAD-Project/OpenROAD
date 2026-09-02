// TARGETS: escaped_port, char_uncovered_punct, depth_2
// CLUE: '<' and '>' in submodule port names. The same string becomes a
// dbModBTerm and a dbModITerm (dbReadVerilog.cc:485,514), is re-read through
// dbModule::findModBTerm -> dbBlock::getBaseName (dbModule.cpp:568), and is
// emitted three times by the writer -- port list, port declaration and the
// `.\i<n (a)` connection (VerilogWriter.cc:190,215). Uncovered characters.
module sub (\i<n , \o>t );
  input \i<n ;
  output \o>t ;
  INV_X1 g1 (.A(\i<n ), .ZN(\o>t ));
endmodule

module top (a, z);
  input a;
  output z;
  sub u1 (.\i<n (a), .\o>t (z));
endmodule

// TARGETS: escaped_port, char_comma, depth_2
// CLUE: ',' separates ports. If writePorts or writePortDcls
// (VerilogWriter.cc:190,215) ever emits this name unescaped the single port
// `\i,n ` silently becomes TWO ports and the module arity changes, which is a
// structurally different failure from a rename. The corpus uses ',' in one
// escaped identifier only, as a top-level net; this is the port kind, on a
// boundary, so the name is emitted in the header, the declaration and the
// `.\i,n (a)` connection.
module sub (\i,n , \o,t );
  input \i,n ;
  output \o,t ;
  INV_X1 g1 (.A(\i,n ), .ZN(\o,t ));
endmodule

module top (a, z);
  input a;
  output z;
  sub u1 (.\i,n (a), .\o,t (z));
endmodule

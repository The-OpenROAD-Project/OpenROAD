// TARGETS: escaped_net, escaped_port, boundary_crossing, depth_2
// CLUE: one electrical net carries THREE different escaped names -- `\t+op ` in
// the top, port `\p-rt ` on the boundary, `\l*oc ` for the module-local
// continuation. In hier mode the crossing net is a dbModNet per side plus one
// flat dbNet, and dbNetwork::checkSanityNetNames (dbNetwork.cc:5709-5737)
// compares net->getName() against each mod_net->getHierarchicalName(); every
// one of those strings is escaped and none of them agree. The corpus's escaped
// port cases reuse the same name on both sides of the boundary.
module sub (\p-rt , o);
  input \p-rt ;
  output o;
  wire \l*oc ;
  INV_X1 g1 (.A(\p-rt ), .ZN(\l*oc ));
  BUF_X1 g2 (.A(\l*oc ), .Z(o));
endmodule

module top (a, z);
  input a;
  output z;
  wire \t+op ;
  INV_X1 g0 (.A(a), .ZN(\t+op ));
  sub u1 (.\p-rt (\t+op ), .o(z));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: hier, module_local_net, escaped_slash_driver_name, name_strip_defeat
// CLUE: dbNetwork::name(Net) (dbNetwork.cc:1854-1866) recovers the in-module net
// name by string-stripping the DRIVER pin's grandparent path off the flat net
// name, and it locates that path with find_last_of('/') -- which is NOT escaped-
// slash aware. A driver instance whose escaped name contains '/' ("g\/1") makes
// the second-to-last '/' land INSIDE the instance name, so header_to_remove
// becomes "u1/g\" and name.find() misses: the net keeps its full flat path.
module top (a, y);
   input a;
   output y;
   m u1 (.i(a), .o(y));
endmodule

module m (i, o);
   input i;
   output o;
   wire w;
   INV_X1 \g/1  (.A(i), .ZN(w));
   BUF_X1 g2 (.A(w), .Z(o));
endmodule

// TARGETS: escaped_net, very_long_name, depth_3
// CLUE: the corpus's longest escaped identifier is 26 chars. This one is 600.
// dbNetwork::pathName(Net) (dbNetwork.cc:2561-2573) accumulates the module
// chain into an fmt::memory_buffer whose INLINE capacity is 500 bytes, so a
// name this long is the first case in the corpus that forces it onto the heap;
// dbNetwork::name(Net) (dbNetwork.cc:2628-2630) then does a substring find and
// an erase on the same string.
module leaf3 (input a, output z);
  wire \w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+ ;
  INV_X1 g1 (.A(a), .ZN(\w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+ ));
  BUF_X1 g2 (.A(\w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+w+ ), .Z(z));
endmodule
module mid3 (input a, output z);
  leaf3 u2 (.a(a), .z(z));
endmodule
module top (input a, output z);
  mid3 u1 (.a(a), .z(z));
endmodule

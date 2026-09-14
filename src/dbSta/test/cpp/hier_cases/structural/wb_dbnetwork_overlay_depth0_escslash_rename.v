// TOP: top
// TECH: nangate45
// TARGETS: hier, no_hierarchy, escaped_slash, name_erase_overshoot
// CLUE: dbNetwork::name(Net)'s strip heuristic runs for ANY dbNet with a '/' in
// its name and no modnet -- including a TOP-LEVEL net in a design with no
// hierarchy at all, because find_last_of('/') counts the escaped slash of
// `\g/y ` (sta "g\/y"). Driver `\g/1 ` gives header_to_remove = "g\", which IS
// found at position 0, so 3 chars are erased and the net should come back
// renamed to plain "y". link_design -hier alone corrupts a flat netlist.
module top (a, y0);
   input a;
   output y0;
   wire \g/y ;
   INV_X1 \g/1  (.A(a), .ZN(\g/y ));
   BUF_X1 g2 (.A(\g/y ), .Z(y0));
endmodule

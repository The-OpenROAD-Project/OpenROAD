// TOP: top
// TECH: nangate45
// TARGETS: hier, no_hierarchy, escaped_slash, name_erase_overshoot, port_capture
// CLUE: same depth-0 rename as ..._depth0_escslash_rename, but the renamed result
// ("y") is the name of a top OUTPUT PORT that already carries a different signal.
// Predict the internal cone gets shorted onto the output port -- a silent logic
// change on a netlist that contains no hierarchy whatsoever.
module top (a, b, y, y1);
   input a;
   input b;
   output y;
   output y1;
   wire \g/y ;
   INV_X1 \g/1  (.A(a), .ZN(\g/y ));
   BUF_X1 g2 (.A(\g/y ), .Z(y1));
   NAND2_X1 g3 (.A1(a), .A2(b), .ZN(y));
endmodule

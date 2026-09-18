// TOP: top
// TECH: nangate45
// TARGETS: hier, degenerate_header, escaped_leading_slash, mid_string_erase
// CLUE: worst case of the unanchored erase. A driver instance named `\/g ` has sta
// name "\/g", so in dbNetwork::name(Net) the instance path's second-to-last '/'
// sits at index 1 and header_to_remove degenerates to a LONE BACKSLASH. name.find
// then matches the escape marker of ANY escaped net name at an arbitrary offset
// and erases 2 chars there: `\a/b ` (sta "a\/b") should be mangled into "ab",
// colliding with the existing net ab.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   wire \a/b ;
   wire ab;
   INV_X1 \/g  (.A(a), .ZN(\a/b ));
   BUF_X1 g2 (.A(\a/b ), .Z(y0));
   INV_X1 g3 (.A(b), .ZN(ab));
   BUF_X1 g4 (.A(ab), .Z(y1));
endmodule

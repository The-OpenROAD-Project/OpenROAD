// TOP: top
// TECH: nangate45
// TARGETS: needs_escape, control, first_char_rule
// CLUE: negative control that pins the exact missing rule: names that CONTAIN digits or
// CLUE: start with '_' are legal bare identifiers, so \_1w and \a1 must round-trip
// CLUE: unescaped and equivalent.  Only the FIRST-char-is-a-digit class is broken.
module top (a, z);
  input a;
  output z;
  wire \_1w ;
  wire \a1 ;
  INV_X1 g (.A(a), .ZN(\_1w ));
  INV_X1 h (.A(\_1w ), .ZN(\a1 ));
  BUF_X1 k (.A(\a1 ), .Z(z));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: bus_range, escaped_brackets, uninitialised_index_bound
// CLUE: bounds the known negative-index defect: isBusName/parseBusName reject a name
// CLUE: whose ']' is preceded by the escape char, and verilogToSta stamps an escape
// CLUE: before every user bracket.  So a user escaped name with a NON-DIGIT subscript
// CLUE: should NOT reach the uninitialised "int index" path -- only reader-generated
// CLUE: bus-bit names can.
module top (a, z);
  input a;
  output z;
  wire \w[x] ;
  wire \ww[] ;
  INV_X1 g (.A(a), .ZN(\w[x] ));
  INV_X1 h (.A(\w[x] ), .ZN(\ww[] ));
  BUF_X1 k (.A(\ww[] ), .Z(z));
endmodule

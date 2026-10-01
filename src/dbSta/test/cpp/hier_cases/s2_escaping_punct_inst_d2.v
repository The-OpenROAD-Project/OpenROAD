// TARGETS: escaped_instance, char_uncovered_punct, depth_2
// CLUE: '?' and '|' in a LEAF instance name one level down. The flat dbInst name
// from dbReadVerilog.cc:538 is "u1/g?1"; the hier path recovers the local name
// with dbNetwork::stripParentPrefix (dbNetwork.cc:1398) and the flat path emits
// the whole joined string through instanceVerilogName. Neither character occurs
// in any existing escaped identifier in the corpus.
module sub (a, y0, y1);
  input a;
  output y0;
  output y1;
  INV_X1 \g?1  (.A(a), .ZN(y0));
  BUF_X1 \g|2  (.A(a), .Z(y1));
endmodule

module top (a, y0, y1);
  input a;
  output y0;
  output y1;
  sub u1 (.a(a), .y0(y0), .y1(y1));
endmodule

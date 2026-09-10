// TOP: top
// TECH: nangate45
// TARGETS: port_order, alias, sort_key
// CLUE: the alphabetized top port list is NOT a port sort -- dbBTerms are created while
// CLUE: walking ConcreteInstanceNetMap (std::map keyed by NET name, dbReadVerilog.cc:705)
// CLUE: and makeTopCell then iterates block getBTerms in creation order.  So the key is
// CLUE: the NET name: output zz whose net is "aaa" must come out BEFORE input i.
module top (i, zz, mm);
  input i;
  output zz;
  output mm;
  wire aaa;
  INV_X1 g1 (.A(i), .ZN(aaa));
  assign zz = aaa;
  BUF_X1 g2 (.A(i), .Z(mm));
endmodule

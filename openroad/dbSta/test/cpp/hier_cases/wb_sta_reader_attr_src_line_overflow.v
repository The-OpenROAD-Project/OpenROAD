// TOP: top
// TECH: nangate45
// TARGETS: attribute, src, stoi, overflow
// CLUE: Verilog2db::storeLineInfo regex-matches a yosys "src" attribute and calls
// CLUE: stoi(match[2]) with an unbounded \d+ group (dbReadVerilog.cc:195,348), so a line
// CLUE: number that does not fit in int throws std::out_of_range out of link_design.
module top (a, y);
  input a;
  output y;
  wire n;
  (* src = "gen.v:99999999999.1-99999999999.9" *) INV_X1 g1 (.A(a), .ZN(n));
  BUF_X1 g2 (.A(n), .Z(y));
endmodule

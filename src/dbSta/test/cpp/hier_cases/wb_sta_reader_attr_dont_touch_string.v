// TOP: top
// TECH: nangate45
// TARGETS: attribute, dont_touch, stoi, linker_crash
// CLUE: VerilogReader.cc:1565-1570 forwards (* *) attributes verbatim, and
// CLUE: dbReadVerilog.cc:580 does std::stoi(network_->getAttribute(child,"dont_touch")).
// CLUE: The canonical spelling of the attribute is the STRING "true", so stoi throws
// CLUE: std::invalid_argument out of link_design.
module top (a, y);
  input a;
  output y;
  wire n;
  (* dont_touch = "true" *) INV_X1 g1 (.A(a), .ZN(n));
  BUF_X1 g2 (.A(n), .Z(y));
endmodule

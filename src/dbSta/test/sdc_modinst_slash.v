// Reproducer: a sub-module instance whose name contains '/' (a Verilog
// escaped identifier).  After read_verilog + link_design -hier, the modInst
// is stored under "i\/sub" so the strtok-style hierarchy walker in
// dbBlock::findModInst splits one name into multiple tokens and misses.
// Without the dbSdcNetwork full-path map fallback, literal SDC patterns
// referencing pins inside such modInsts silently drop with STA-0363.

module sub (clk, out);
  input clk;
  output out;
  snl_bufx1 buf1 (.A(clk), .Z(out));
endmodule

module top (clk, out);
  input clk;
  output out;
  sub \i/sub (.clk(clk), .out(out));
endmodule

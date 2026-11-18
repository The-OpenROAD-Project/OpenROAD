// TOP: top
// TECH: nangate45
// TARGETS: needs_escape, leading_digit, module_name, hier
// CLUE: cellVerilogName -> staToVerilog, so a module whose escaped name starts with a
// CLUE: digit is emitted as "module 1m (" in hier mode and as the instance cell name.
module \1m (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule

module top (a, y);
  input a;
  output y;
  \1m  u (.i(a), .o(y));
endmodule

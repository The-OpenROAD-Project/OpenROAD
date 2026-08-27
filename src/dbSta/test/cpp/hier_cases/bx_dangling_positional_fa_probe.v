// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, five_pin_cell, offset_probe, dead_cone
// CLUE: five-pin liberty cell positional: FA_X1 g (p1,p2,p3,co,s). If the
// binding is shifted by 2 (pin[k] <- arg[k+2]) then A<-p3, B<-co, CI<-s and
// both outputs stay open. Reads the offset off a wide cell.
module top (input p1, input p2, input p3, output y);
  wire co;
  wire s;
  wire dco;
  wire ds;
  INV_X1 keep (.A(p1), .ZN(y));
  FA_X1 g2 (p1, p2, p3, co, s);
  INV_X1 sink_co (.A(co), .ZN(dco));
  INV_X1 sink_s (.A(s), .ZN(ds));
endmodule

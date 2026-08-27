// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, depth_3
// CLUE: the badly sorted receiving wire sits at the TOP of a depth-3 feedthrough chain: does the same name pivot break a deeper hierarchy?

module leaf (input a, output y);
  assign y = a;
endmodule

module mid (input ma, output my);
  leaf u_l (.a(ma), .y(my));
endmodule

module top (input i, output o);
  wire a_alias;
  mid u0 (.ma(i), .my(a_alias));
  assign o = a_alias;
endmodule

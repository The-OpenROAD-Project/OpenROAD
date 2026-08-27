// TOP: top
// TECH: nangate45
// TARGETS: clock_from_top_data_input, same_net_clock_and_data
// CLUE: top input `a` is the flop's clock AND a data input of a gate in another
// submodule -- nothing marks it as a clock, so clock-aware code may misfire.

module ffmod (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module comb (input x, input y, output z);
  AND2_X1 g (.A1(x), .A2(y), .ZN(z));
endmodule

module top (input a, input d, output q, output z);
  wire qi;
  ffmod u0 (.d(d), .ck(a), .q(qi));
  comb  u1 (.x(a), .y(d), .z(z));
  assign q = qi;
endmodule

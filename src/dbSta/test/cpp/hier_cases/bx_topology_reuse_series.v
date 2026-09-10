// TOP: top
// TECH: nangate45
// TARGETS: module_reuse, series_instances_same_parent
// CLUE: two instances of the same module wired in SERIES inside one parent
// (output of first feeds input of second); internal net between siblings
// must survive both paths.

module stg (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module duo (input i, output o);
  wire m;
  stg u1 (.i(i), .o(m));
  stg u2 (.i(m), .o(o));
endmodule

module top (input a, output z);
  duo d (.i(a), .o(z));
endmodule

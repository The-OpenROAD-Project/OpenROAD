// TOP: top
// TECH: nangate45
// TARGETS: scalar_feedthrough, two_readers, concat_and_direct
// CLUE: minimised gp_no_bus_ft: scalar feedthrough wire read BOTH inside a concat assign to an output bus and directly by a scalar output assign.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input k, output [1:0] o, output p);
  wire m;
  ft u0 (.a(i), .y(m));
  assign o = {k, m};
  assign p = m;
endmodule

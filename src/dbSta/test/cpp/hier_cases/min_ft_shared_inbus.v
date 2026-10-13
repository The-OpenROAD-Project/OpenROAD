// TOP: top
// TECH: nangate45
// TARGETS: scalar_feedthrough, shared_input_bus, two_readers
// CLUE: the concat's other bit comes from the SAME top input bus that feeds the sub, so a top input bit and the sub feedthrough net belong to one alias group.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input [1:0] i, output [1:0] o, output p);
  wire m;
  ft u0 (.a(i[1]), .y(m));
  assign o = {i[0], m};
  assign p = m;
endmodule

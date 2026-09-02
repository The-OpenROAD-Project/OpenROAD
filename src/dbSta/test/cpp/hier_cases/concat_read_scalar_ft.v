// TOP: top
// TECH: nangate45
// TARGETS: concat_read_of_feedthrough_wire, scalar_feedthrough
// CLUE: suspect T2 with a SCALAR feedthrough sub: is a bus needed inside the child at all, or is the top-level concat read enough?

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input k, output [1:0] o);
  wire m;
  ft u0 (.a(i), .y(m));
  assign o = {k, m};
endmodule

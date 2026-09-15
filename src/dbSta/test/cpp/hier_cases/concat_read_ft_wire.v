// TOP: top
// TECH: nangate45
// TARGETS: concat_read_of_feedthrough_wire, submodule_feedthrough
// CLUE: suspect T2 minimal: whole-bus feedthrough sub, output on an internal top wire that is read inside a top-level CONCAT assign (bit-reversing) to the output bus.

module ft (input [1:0] a, output [1:0] y);
  assign y = a;
endmodule

module top (input [1:0] i, output [1:0] o);
  wire [1:0] m;
  ft u0 (.a(i), .y(m));
  assign o = {m[0], m[1]};
endmodule

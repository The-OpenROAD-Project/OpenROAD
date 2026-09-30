// TOP: top
// TECH: nangate45
// TARGETS: submodule_full_bus_feedthrough_assign
// CLUE: submodule whose only content is a full-bus feedthrough assign
// (assign o = i on 4-bit ports); probes around the known flat finding that
// drops bus-SLICE feedthrough assigns — is the full-bus form also dropped?

module fb (input [3:0] i, output [3:0] o);
  assign o = i;
endmodule

module top (input [3:0] a, output [3:0] z);
  fb u (.i(a), .o(z));
endmodule

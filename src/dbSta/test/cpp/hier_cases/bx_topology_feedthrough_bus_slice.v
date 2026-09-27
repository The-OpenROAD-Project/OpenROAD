// TOP: top
// TECH: nangate45
// TARGETS: submodule_bus_slice_feedthrough_assign, known_finding_2_probe
// CLUE: submodule containing only bus-slice feedthrough assigns crossing the
// halves (o[1:0]=i[3:2], o[3:2]=i[1:0]); expected to reproduce the known
// flat-path dropped-assign finding (get_ports1.v pattern).

module fs (input [3:0] i, output [3:0] o);
  assign o[1:0] = i[3:2];
  assign o[3:2] = i[1:0];
endmodule

module top (input [3:0] a, output [3:0] z);
  fs u (.i(a), .o(z));
endmodule

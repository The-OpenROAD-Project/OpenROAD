// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign_drives_submodule_output_port, register_bank
// CLUE: inside the submodule the flop outputs reach the output port only
// through bus-slice assigns, and that port is wired straight to the top output
// bus -- if the slice assigns are dropped the top outputs lose their drivers.

module bankft (input [3:0] d, input ck, input rn, output [3:0] q);
  wire [3:0] s;
  DFFR_X1 f0 (.D(d[0]), .RN(rn), .CK(ck), .Q(s[0]));
  DFFR_X1 f1 (.D(d[1]), .RN(rn), .CK(ck), .Q(s[1]));
  DFFR_X1 f2 (.D(d[2]), .RN(rn), .CK(ck), .Q(s[2]));
  DFFR_X1 f3 (.D(d[3]), .RN(rn), .CK(ck), .Q(s[3]));
  assign q[1:0] = s[1:0];
  assign q[3:2] = s[3:2];
endmodule

module top (input [3:0] d, input ck, input rn, output [3:0] q);
  bankft u (.d(d), .ck(ck), .rn(rn), .q(q));
endmodule

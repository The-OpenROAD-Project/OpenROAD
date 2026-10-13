// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_feedthrough_module_between_two_banks
// CLUE: the module between the two 4-bit banks contains only bus-slice
// feedthrough assigns; if those are dropped, the second bank loses its D.

module ftb (input [3:0] a, output [3:0] z);
  assign z[1:0] = a[1:0];
  assign z[3:2] = a[3:2];
endmodule

module bank (input [3:0] d, input ck, input rn, output [3:0] q);
  DFFR_X1 f0 (.D(d[0]), .RN(rn), .CK(ck), .Q(q[0]));
  DFFR_X1 f1 (.D(d[1]), .RN(rn), .CK(ck), .Q(q[1]));
  DFFR_X1 f2 (.D(d[2]), .RN(rn), .CK(ck), .Q(q[2]));
  DFFR_X1 f3 (.D(d[3]), .RN(rn), .CK(ck), .Q(q[3]));
endmodule

module top (input [3:0] d, input ck, input rn, output [3:0] q);
  wire [3:0] s, t;
  bank b0 (.d(d), .ck(ck), .rn(rn), .q(s));
  ftb  f  (.a(s), .z(t));
  bank b1 (.d(t), .ck(ck), .rn(rn), .q(q));
endmodule

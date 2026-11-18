// TOP: top
// TECH: nangate45
// TARGETS: shared_net, same_cell_two_pins, depth_1
// CLUE: Inside a submodule, one net drives both pins of a NAND2 (a degenerate
// CLUE: inverter). Both cell pins reference the identical net.

module top (a, y);
 input a;
 output y;
 sub u (.p(a), .z(y));
endmodule

module sub (p, z);
 input p;
 output z;
 wire m;
 INV_X1 n (.A(p), .ZN(m));
 NAND2_X1 g (.A1(m), .A2(m), .ZN(z));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: header_port_slice, unnamed_port, bus_slice
// CLUE: The header names a PART-SELECT of a declared bus as a port:
// CLUE: module sub (i[1:0], o); - legal port_reference with a range.

module top (a, y);
 input [1:0] a;
 output y;
 sub u (a, y);
endmodule

module sub (i[1:0], o);
 input [3:0] i;
 output o;
 NAND2_X1 g (.A1(i[0]), .A2(i[1]), .ZN(o));
endmodule

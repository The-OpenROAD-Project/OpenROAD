// TOP: top
// TECH: nangate45
// TARGETS: instance_array, positional_split, probe
// CLUE: PROBE: an array of instances h2 u [1:0] whose 2-bit ports are filled
// CLUE: by successive slices of the 4-bit parent buses.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 h2 u [1:0] (.i(a), .o(y));
endmodule

module h2 (i, o);
 input [1:0] i;
 output [1:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
endmodule

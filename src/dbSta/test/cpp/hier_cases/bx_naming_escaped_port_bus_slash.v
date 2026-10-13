// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, bus_port, char_slash, depth_1
// CLUE: top-level BUS port named \p/q  (wire [3:0]); slash in a bus port
// name plus bit-selects \p/q [i] stress both reader and writer.
module top (input [3:0] a, output [3:0] \p/q );
  BUF_X1 b0 (.A(a[0]), .Z(\p/q [0]));
  BUF_X1 b1 (.A(a[1]), .Z(\p/q [1]));
  BUF_X1 b2 (.A(a[2]), .Z(\p/q [2]));
  BUF_X1 b3 (.A(a[3]), .Z(\p/q [3]));
endmodule

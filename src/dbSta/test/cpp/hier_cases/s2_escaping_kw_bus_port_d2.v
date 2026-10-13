// TARGETS: escaped_port, keyword_input, bus, depth_2
// CLUE: keyword-named BUS PORT on a submodule. writePortDcls prints
// `input [1:0] {portVerilogName(name)}` (VerilogWriter.cc:215-225) and
// staToVerilog2 (VerilogNamespace.cc:120-158) leaves "input" unescaped because
// every character is alnum, so the boundary should be emitted as
// `input [1:0] input;` plus a `.input(...)` connection. The covered case
// (bx_naming_escaped_port_kw_input.v) is a SCALAR port on the top module; this
// one crosses a hierarchy boundary and takes the bus declaration path.
module sub (\input , o);
  input [1:0] \input ;
  output o;
  AND2_X1 g1 (.A1(\input [0]), .A2(\input [1]), .ZN(o));
endmodule

module top (a, z);
  input [1:0] a;
  output z;
  sub u1 (.\input (a), .o(z));
endmodule

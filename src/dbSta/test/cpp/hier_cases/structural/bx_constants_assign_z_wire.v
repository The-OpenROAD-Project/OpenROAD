// TOP: top
// TECH: nangate45
// TARGETS: assign_const_wire, z_value, probe_reader
// CLUE: assign w = 1'bz then w consumed by a gate — z in assign context
// (pin-context 1'bz already probed by bx_constants_z_conn); record what the
// reader/writer do with a high-impedance constant driver.
module top (input a, output y);
  wire w;
  assign w = 1'bz;
  OR2_X1 g (.A1(a), .A2(w), .ZN(y));
endmodule

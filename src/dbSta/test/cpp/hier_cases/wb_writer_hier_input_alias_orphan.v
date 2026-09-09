// TOP: top
// TECH: nangate45
// TARGETS: assign_emission, input_port_alias, isAnyOutput_filter, hier
// CLUE: writeAssigns only emits an alias when the port isAnyOutput
// CLUE: (VerilogWriter.cc:456), so an INPUT port whose net lost the naming contest gets
// CLUE: no assign and the net it feeds has no driver.  mergeAssignNet keeps the
// CLUE: shallower net (VerilogReader.cc:1854), so sub's internal "aa" becomes the
// CLUE: top-level name while sub's formal stays "i" -- inside module sub they are then
// CLUE: two different objects with nothing joining them.
module sub (i, o);
  input i;
  output o;
  wire aa;
  assign aa = i;
  INV_X1 g (.A(aa), .ZN(o));
endmodule

module top (p, z);
  input p;
  output z;
  sub u (.i(p), .o(z));
endmodule

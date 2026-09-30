// TOP: top
// TECH: nangate45
// TARGETS: same_scope_names, inst_named_as_top_module
// CLUE: leaf instance NAMED top inside module top; instance name equals the
// enclosing (and linked) module name -- legal per LRM, probes linker/writer
// symbol lookup that keys on the name top.
module top (input in1, output o1);
  INV_X1 top (.A(in1), .ZN(o1));
endmodule

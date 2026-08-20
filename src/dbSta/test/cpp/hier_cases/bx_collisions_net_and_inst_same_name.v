// TOP: top
// TECH: nangate45
// TARGETS: same_scope_names, net_vs_inst, reader_probe
// CLUE: wire x and instance x in one module; the LRM puts nets and instances
// in ONE module namespace, so this is illegal input -- probes whether the
// reader rejects it cleanly or silently corrupts.
module top (input in1, output o1);
  wire x;
  INV_X1 x (.A(in1), .ZN(x));
  INV_X1 g2 (.A(x), .ZN(o1));
endmodule

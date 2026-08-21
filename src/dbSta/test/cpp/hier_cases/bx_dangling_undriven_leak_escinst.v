// TOP: top
// TECH: nangate45
// TARGETS: undriven_net_in_module, name_leak, escaped_instance_name
// CLUE: the instance whose path leaks into sub's undriven net name is itself
// escaped and already contains a slash: \a/b . The leaked module-local net
// name becomes \a/b/und , indistinguishable from a two-level path.
module sub (input a, output y);
  wire und;
  wire d;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(und), .ZN(d));
endmodule
module top (input in1, output out1);
  sub \a/b (.a(in1), .y(out1));
endmodule

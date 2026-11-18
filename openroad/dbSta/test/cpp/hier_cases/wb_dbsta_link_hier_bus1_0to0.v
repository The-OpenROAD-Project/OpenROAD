// TOP: top
// TECH: nangate45
// TARGETS: single_bit_bus_port, dbBusPort_from_eq_to, hier
// CLUE: makeModBTerms computes size = from-to+1 and updown = from<=to, so a
// one-bit bus [0:0] takes the ascending branch with size 1 and the same
// modbterm becomes both setMembers and setLast of the dbBusPort. The aggregate
// sentinel is named "d" while the only bit is "d[0]"; the push-down loop then
// calls findNet(inst,"d") for the sentinel, which is exactly the aggregate-pin
// confusion makeModITerms warns about.
module top (a, y);
   input a;
   output y;
   sub u (.d(a), .q(y));
endmodule

module sub (d, q);
   input [0:0] d;
   output q;
   BUF_X1 g (.A(d[0]), .Z(q));
endmodule

// TOP: top
// TECH: nangate45
// TARGETS: escaped_slash_bus_port, dbBusPort_hijack, modbterm_tail_collision
// CLUE: bus flavour of the tail collision. makeModBTerms builds bus bit names
// as <port>[i], so the escaped bus `\x/y [1:0]` yields "x\/y", "x\/y[1]",
// "x\/y[0]" -- each of which tail-matches the ports of bus y[1:0] that was
// created first. dbBusPort::create then re-parents y's aggregate sentinel to
// the new bus port (setBusPort), so one bus port should swallow the other.
module top (a, b, o);
   input [1:0] a;
   input [1:0] b;
   output [1:0] o;
   sub u (.y(a), .\x/y (b), .o(o));
endmodule

module sub (y, \x/y , o);
   input [1:0] y;
   input [1:0] \x/y ;
   output [1:0] o;
   AND2_X1 g0 (.A1(y[0]), .A2(\x/y [0]), .ZN(o[0]));
   AND2_X1 g1 (.A1(y[1]), .A2(\x/y [1]), .ZN(o[1]));
endmodule

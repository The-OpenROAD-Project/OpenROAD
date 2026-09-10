// TOP: top
// TECH: nangate45
// TARGETS: hier, findPort_tail_fallback, missing_wire_dcl
// CLUE: VerilogWriter::writeWireDcls skips the wire declaration when
// network_->findPort(cell, net_name) != nullptr, and dbNetwork::findPort routes to
// dbModule::findModBTerm, which FALLS BACK to the substring after the last '/'
// (dbModule.cpp:556-577). A module-local net named `\x/o ` therefore "matches"
// the module's port o, so its declaration is suppressed while the connections
// still reference `\x/o `.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   m u1 (.i(a), .j(b), .o(y0), .q(y1));
endmodule

module m (i, j, o, q);
   input i;
   input j;
   output o;
   output q;
   wire \x/o ;
   INV_X1 g1 (.A(i), .ZN(\x/o ));
   BUF_X1 g2 (.A(\x/o ), .Z(q));
   BUF_X1 g3 (.A(j), .Z(o));
endmodule

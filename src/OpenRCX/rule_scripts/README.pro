
parameter.1   : contains original numbers of the process tables from customer 
process.stack : contains nominal process stack widtout variability from customer

M1 		: contains all related numbers for variability for M1
			dtr, dtc, dwc, dwr, alpha, x, p
M2 		: contains all related numbers for variability for M2-3-4-5
			dtr, dtc, dwc, dwr, alpha, x, p

w.tcl		: tcl file that reads dirs M1 and M2 and creates files:
			lo_cw.out - lo width for C
			hi_cw.out - hi width for C
			lo_rw.out - lo width for R
			hi_rw.out - hi width for R
			cth.out - thickness for C
			rth.out - thickness for R

w.tcl converts the process tables into Nefelus Variability Tables

to run w.tcl you do >> tclsh w.tcl
to understand w.tcl, start reading where it says "foreach m { M1 ..."



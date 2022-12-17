from openroad import Design, Tech
import odb

tech = Tech()
design = Design(tech)

design.evalTclString("read_db gcd_fill.odb; read_sdc gcd_fill.sdc")
design.evalTclString("set_propagated_clock [all_clocks]")

dfl = design.getFinale()

dfl.densityFill("fill.json", design.getBlock().getCoreArea())


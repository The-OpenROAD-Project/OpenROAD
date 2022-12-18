from openroad import Design, Tech
import odb

tech = Tech()
design = Design(tech)

design.readDb("gcd_fill.odb")
dfl = design.getFinale()

dfl.densityFill("fill.json", design.getBlock().getCoreArea())

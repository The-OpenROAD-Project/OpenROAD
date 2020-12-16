import opendbpy as odb
import os 

current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")

db = odb.dbDatabase.create()
lib = odb.read_lef(db, os.path.join(data_dir, "Nangate45/NangateOpenCellLibrary.mod.lef"))
odb.read_def(db, os.path.join(data_dir, "gcd/floorplan.def"))
chip = db.getChip()
tech = db.getTech()
block = chip.getBlock()
sites = lib.getSites()
site = sites[0]

rt = odb.dbRow_create(block,"ROW_TEST", site, 0, 380, "MX", "HORIZONTAL", 420, 380)

assert rt.getName() == "ROW_TEST", "Rown name mismatch"
assert rt.getOrigin() == [0,380], "Row origin mismatch"
assert rt.getSite().getName() == site.getName(), "Row Site name mismatch"
assert rt.getDirection() == "HORIZONTAL", "Row diretion mismatch"
assert rt.getOrient() == "MX", "Row orientation mismatch"
assert rt.getSpacing() == 380, "Row spacing mismatch"
assert rt.getSiteCount() == 420, "row site count mismatch"

rt1 = odb.dbRow_create(block, "ROW_TEST", site, 0, 380, "R0", "HORIZONTAL", 420, 380)
assert rt1.getDirection() == "HORIZONTAL" , "Row get direction mismatch"
assert rt1.getOrient() == "R0", "Row orientation mismatch"

rt2 = odb.dbRow_create(block, "ROW_TEST", site, 0, 380, "R0", "VERTICAL", 420, 380)
assert rt2.getDirection() == "VERTICAL", "Row get direction mismatch"
assert rt2.getOrient() == "R0", "Row get orientation mismatch"

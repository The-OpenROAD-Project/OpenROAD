import opendbpy as odb
import os 

current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")

db = odb.dbDatabase.create()
odb.read_lef(db, os.path.join(data_dir, "Nangate45","NangateOpenCellLibrary.mod.lef"))
odb.read_lef(db, os.path.join(data_dir, "ICEWall","dummy_pads.lef"))
odb.read_def(db, os.path.join(data_dir, "ICEWall","octilinear.def"))
chip = db.getChip()
if chip == None:
    exit("Read DEF Failed")
result = odb.write_def(chip.getBlock(), os.path.join(opendb_dir, "build","generated_octilinear.def"))
assert result==1, "DEF not written"

db_file = os.path.join(opendb_dir, "build","export_oct.db")
export_result = odb.write_db(db, db_file)
if export_result!=1:
    exit("Export DB Failed")
    
new_db = odb.dbDatabase.create()
new_db = odb.read_db(new_db, db_file)

if odb.db_diff(db, new_db):
    exit("Error: Difference found between exported and imported DB")


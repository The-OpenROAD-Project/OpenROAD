import opendbpy as odb
import os 

current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")

db = odb.dbDatabase.create()
odb.read_lef(db, os.path.join(data_dir, "gscl45nm.lef"))
tech = db.getTech()
vias = tech.getVias()
for via in vias:
    via.getName()
assert len(vias) == 14, "Number of vias mismatch"

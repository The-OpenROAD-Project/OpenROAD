import opendbpy as odb
import os 

current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")

db = odb.dbDatabase.create()
lib = odb.read_lef(db, os.path.join(data_dir, "gscl45nm.lef"))
odb.read_def(db, os.path.join(data_dir, "design.def"))
chip = db.getChip()
tech = db.getTech()

block = chip.getBlock()
net = odb.dbNet_create(block, "w1")

net.setSigType("POWER")
swire = odb.dbSWire_create(net, "ROUTED")
if (swire.getNet().getName() != net.getName()):
    exit("ERROR: Net and signal wire mismatch")

sites = lib.getSites()
site = sites[0]
row = odb.dbRow_create(block, "row0", site, 0, 0, "RO", "HORIZONTAL", 1, 10)
if row == None:
    exit("ERROR: Row not created")
print(net)
print(row)
print(swire)



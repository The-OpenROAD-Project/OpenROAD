import opendbpy as odb
import os 
current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")


db = odb.dbDatabase.create()
lef = odb.read_lef(db, os.path.join(data_dir, "gscl45nm.lef"))
dbTech = db.getTech()
cutLayer = dbTech.findLayer("via1")
defs = cutLayer.getTechLayerCutSpacingTableDefRules()

# Test for std::vector< std::pair< T*, T* > >
prlFACtbl = defs[0].getPrlForAlignedCutTable()
assert(len(prlFACtbl) == 2)
assert(prlFACtbl[0][0].getName() == "cls1")
assert(prlFACtbl[0][1].getName() == "cls2")
assert(prlFACtbl[1][0].getName() == "cls3")
assert(prlFACtbl[1][1].getName() == "cls4")

# Test for std::vector< std::tuple< T*, int, int, int > >
opoEnctbl = defs[0].getOppEncSpacingTable()
assert(len(opoEnctbl) == 0)

# Test for std::vector< std::pair< T*, int > >
alignedTable = defs[0].getExactAlignedTable()
assert(len(alignedTable) == 0)

# Test for std::pair<int,int>
spacing1 = defs[0].getSpacing("cls1", True, "cls3", False)
assert(spacing1[0] == 0.1 * 2000)
assert(spacing1[1] == 0.2 * 2000)

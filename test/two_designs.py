import os
import helpers
from openroad import Design, Tech
import odb

ord_tech1 = Tech()
design1 = Design(ord_tech1)
design1.readDb("../src/odb/test/data/design.odb")

db2 = Design.createDetachedDb()
db2 = odb.read_db(db2, "../src/odb/test/data/design.odb")

db1_file = helpers.make_result_file("db1.odb")
db2_file = helpers.make_result_file("db2.odb")

design1.writeDb(db1_file)
odb.write_db(db2, db2_file)

if not os.path.isfile(db1_file):
    raise Exception(f"file missing {db1_file}")

if not os.path.isfile(db2_file):
    raise Exception(f"file missing {db2_file}")

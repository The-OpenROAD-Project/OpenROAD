import os
import helpers
from openroad import Design, Tech
import odb

helpers.if_bazel_change_working_dir_to("/_main/test/")

design_odb = helpers.get_runfiles_path_to("/_main/src/odb/test/data/design.odb")
if not design_odb:
    design_odb = "../src/odb/test/data/design.odb"

ord_tech1 = Tech()
design1 = Design(ord_tech1)
design1.readDb(design_odb)

# In the standalone Bazel Python path, Design.createDetachedDb() crashes.
# Use the detached ODB read helper directly instead.
db2 = odb.read_db(None, design_odb)

db1_file = helpers.make_result_file("db1.odb")
db2_file = helpers.make_result_file("db2.odb")

design1.writeDb(db1_file)
odb.write_db(db2, db2_file)

if not os.path.isfile(db1_file):
    raise Exception(f"file missing {db1_file}")

if not os.path.isfile(db2_file):
    raise Exception(f"file missing {db2_file}")

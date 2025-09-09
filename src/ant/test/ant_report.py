import helpers

# A minimal LEF file that has been modified to include particular
# antenna values for testing
from openroad import Design, Tech
import ant

tech = Tech()
tech.readLef("ant_check.lef")

design = helpers.make_design(tech)
design.readDef("ant_check.def")
ack = design.getAntennaChecker()

# set report file
reportFile = helpers.make_result_file("ant_report.rpt")
ack.setReportFileName(reportFile)

ack.checkAntennas(verbose=False)

diff = helpers.diff_files("ant_report.rptok", reportFile)

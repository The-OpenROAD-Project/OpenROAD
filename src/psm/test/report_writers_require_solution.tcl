# The report writers derive their output from a solution, so each must say so when
# there isn't one -- and must say it before creating a file.
#
# write_pg_spice called on its own used to throw a bare `map::at` from inside the
# solver, after having already written the resistive network. That left a
# syntactically valid spice deck with no sources and no sinks: a floating network a
# simulator will solve to nonsense rather than reject.
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/gcd.sdc

set spice_file [make_result_file report_writers_require_solution.sp]
set voltage_file [make_result_file report_writers_require_solution-voltage.rpt]
set em_file [make_result_file report_writers_require_solution-em.rpt]

# make_result_file only builds a path, so output from an earlier run in the same
# results/ is still present. The checks below report whether each writer created
# its file, which only means anything when the files start out absent.
file delete -force $spice_file $voltage_file $em_file

catch { write_pg_spice -net VDD -vsrc Vsrc_gcd_vdd.loc $spice_file } err
puts $err
puts "spice file written before analysis: [file exists $spice_file]"

# and once a solution exists, all three writers work
analyze_power_grid -net VDD -vsrc Vsrc_gcd_vdd.loc \
  -voltage_file $voltage_file -enable_em -em_outfile $em_file
write_pg_spice -net VDD -vsrc Vsrc_gcd_vdd.loc $spice_file

puts "spice file written after analysis: [file exists $spice_file]"
puts "voltage file written: [file exists $voltage_file]"
puts "em file written: [file exists $em_file]"

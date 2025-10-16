top_dir=$(shell pwd)
# top_dir=$(top_dir1)
data_dir=$(top_dir)/data
# @echo "Top Dir = $(top_dir)"
# -------------------------------------------------------------------------------------
#                        rcx model file generation steps
# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------
# REQUIRED Manual actions:
#       create link to openroad executable: 
#			ln -s /home/dimitris-ic/z/v2_rcx/OpenROAD-OpenRCX-v2-fotakis/build/src/openroad bin/
#	Copy FasterCap executable into bin
#	Copy process.TYP files onto dir data: one process.TYP per corner
# -------------------------------------------------------------------------------------

gen_pattern_bash=$(top_dir)/scripts/run_patterns.bash
gen_patterns=$(top_dir)/scripts/gen_patterns.bash
script_dir=$(top_dir)/scripts
openroad=$(top_dir)/bin/openroad

# -------------------------------------------------------------------------------------
#                        rcx generates patterns
# -------------------------------------------------------------------------------------

# gen_patterns script usage: run_dir openroad_exec input_process_file corner_name wire_cnt 
#                                              version(1=not normalized, 2=normalized heights)
# ---------------------------------------------------------------------------------------------

5v2_min: 
	$(gen_patterns) $@ $(openroad) $(data_dir)/process.MIN MIN 5 2 ;  \
	echo " --- Wire Patterns Directory: $@"

5v2_typ: 
	$(gen_patterns) $@ $(openroad) $(data_dir)/process.TYP TYP 5 2 ; \
	echo " --- Wire Patterns Directory: $@"


# -------------------------------------------------------------------------------------
#                        Run fasterCap
# -------------------------------------------------------------------------------------
run_fasterCap=$(top_dir)/scripts/run_fasterCap.bash
rcx2fasterCap=$(top_dir)/scripts/UniversalFormat2FasterCap_923.py
fasterCap=$(top_dir)/bin/FasterCap

5v2_typ_fasterCap:
	$(run_fasterCap) 5v2_typ $@ standard 20 ALL $(rcx2fasterCap) $(fasterCap)

5v2_min_fasterCap:
	$(run_fasterCap) 5v2_min $@ standard 20 ALL $(rcx2fasterCap) $(fasterCap)

# -------------------------------------------------------------------------------------
#                        fasterCapParse parse of fasterCap Output
# -------------------------------------------------------------------------------------
parse_fasterCap_bash=$(top_dir)/scripts/parse_fasterCap.bash
fasterCapParsePython=$(top_dir)/scripts/fasterCapParse.py

5v2_typ_parse:
	$(parse_fasterCap_bash) $@  $(top_dir) 5v2_typ $(fasterCapParsePython) 3
	
5v2_min_parse:
	$(parse_fasterCap_bash) $@  $(top_dir) 5v2_min $(fasterCapParsePython) 3
	

# -------------------------------------------------------------------------------------
#                        rcx generates model file
# -------------------------------------------------------------------------------------
readCaps_tcl=readCaps.rcx.tcl

model: 
	echo $@ ; \
        rm -rf $@ ; \
        mkdir $@ ; \
        cd $@ ; \
	echo "init_rcx_model -corner_names \"TYP MIN\" -met_cnt 7" > $(readCaps_tcl) ; \
	echo "read_rcx_tables -corner TYP -file  $(top_dir)/5v2_typ_parse/5v2_typ.caps " >>  $(readCaps_tcl) ; \
	echo "read_rcx_tables -corner TYP -file  $(top_dir)/5v2_typ/resistance.TYP " >>  $(readCaps_tcl) ; \
	echo "read_rcx_tables -corner TYP -file  $(top_dir)/5v2_min_patrse/5v2_typ.caps " >>  $(readCaps_tcl) ; \
	echo "read_rcx_tables -corner TYP -file  $(top_dir)/5v2_min/resistance.TYP " >>  $(readCaps_tcl) ; \
	echo "write_rcx_model -file 130.rcx.model " >> $(readCaps_tcl) ; \
	$(openroad) < $(readCaps_tcl) > OUT ; \
	# echo "$@ diffs: `diff -w -r . ../gen924.GOLD` "


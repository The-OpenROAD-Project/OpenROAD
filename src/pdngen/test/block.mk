check: $(BLOCK)_pdn.def
	@diff -q $< $(BLOCK)_pdn.check && echo "Success"

test: $(BLOCK)_pdn.def

approve: $(BLOCK)_pdn.check

$(BLOCK).fp.odb :
	opendbtcl ../init_db.tcl

$(BLOCK)_pdn.def : $(BLOCK).fp.odb PDN.cfg
	opendbtcl ../apply_pdn.tcl

$(BLOCK)_pdn.check: $(BLOCK)_pdn.def
	cp $< $@

clean:
	-@rm $(BLOCK)_pdn.def
	-@rm $(BLOCK).fp.odb
	-@rm $(BLOCK).geom.rpt
	-@rm floorplan.def.v


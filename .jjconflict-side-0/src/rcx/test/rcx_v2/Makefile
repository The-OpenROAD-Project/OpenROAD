
.PHONY :  flow model

all :  flow model corners fasterCap fasterCap-gen

flow:
	cd $@ ; make all

corners:
	cd flow/$@ ; make all

model:
	cd $@ ; make patterns

fasterCap:
	cd FasterCapModel ; make patterns

fasterCap-gen:
	cd FasterCapModel ; make gen924



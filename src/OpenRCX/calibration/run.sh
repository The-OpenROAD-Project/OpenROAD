# Clean up directory
rm -rf work
mkdir -p data
mkdir -p work 
cd work

# Create a soft link to openroad executable
ln -s ../../../../build/src/openroad

# Step A: Generate Patterns Layout that models
#         various capacitance and resistance
#         models.
./openroad ../script/generate_patterns.tcl

# Step B: Perform extraction using reference extractor.
# The output parasitics file should placed in the 
# directory below.
cd ./EXT

##################################
# Running the reference Extractor
# #Put Command below 
##################################

################################
cd ..

# Step C: Generate OpenRCX tech file 
#         (OpenRCX RC table) by converting
#         the parasitics extracted from the
#         reference extractor to a RC table.
./openroad ../script/generate_rules.tcl

# Step D: Perform parasitic extraction on the 
#         pattern geometries, and compare the 
#         parasitic result with the golden parasitics 
#         calculated by reference extractor to evaluate 
#         the accuracy of the OpenRCX using the generated
#         RC table (Extraction Rule file).
./openroad ../script/ext_patterns.tcl

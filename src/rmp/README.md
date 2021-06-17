### Restructure
**Restructure** is an interface to ABC for re-synthesis. The package allows
re-structuring targetting area or timing. It extracts cloud of logic using
timing engine and passes to ABC through blif interface. Multiple recipes
for area or timing are run to get best desirable structure from ABC.
ABC output is read back by blif reader which is integrated to OpenDB.
Blif writer and reader also supports constants from and to OpenDB. Reading
back of constants required insertion of tie cells which should be provided
by user as per interface given below.


## Usage
Restructuring can be done in 2 modes
# Area Mode
restructure -liberty_file <liberty_file> -mode "area" \
        -locell  tie_low_cell -loport tie_low_cell_port \
        -hicell  tie_high_cell -hiport tie_high_port

# Timing Mode
restructure -liberty_file <liberty_file> -mode "delay" \
        -slack_threshold slack_val -depth_threshold depth_threshold\
        -locell  tie_low_cell -loport tie_low_cell_port \
        -hicell  tie_high_cell -hiport tie_high_port

Argument Description
- ``liberty_file`` Liberty file with description of cells used in design. This would be passed to ABC.
- ``mode`` could be area or delay. In area mode focus is area reduction and timing may degrade. In delay mode delay would be reduced but area may increase.
- ``-slack_threshold`` specifies slack value below which timing paths need to be analyzed for restructuring
- ``-depth_threshold`` specifies the path depth above which a timing path would be considered for restructuring
- ``locell`` specifies tie cell which can drive constant zero
- ``hicell`` specifies tie cell which can drive constant one
 
### Authors
Sanjiv Mathur
Ahmad El Rouby

## License ##
* [BSD 3-clause License](LICENSE) 


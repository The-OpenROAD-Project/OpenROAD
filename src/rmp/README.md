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
Restructuring can be done in 2 modes area or delay
# Area Mode
restructure -liberty_file <liberty_file> -target "area" \
        -tielo_pin  tielo_pin_name -tiehi_pin  tiehi_pin_name

# Timing Mode
restructure -liberty_file <liberty_file> -target "delay" \
        -slack_threshold slack_val -depth_threshold depth_threshold\
        -tielo_pin  tielo_pin_name -tiehi_pin  tiehi_pin_name

Argument Description
- ``liberty_file`` Liberty file with description of cells used in design. This would be passed to ABC.
- ``target`` could be area or delay. In area mode focus is area reduction and timing may degrade. In delay mode delay would be reduced but area may increase.
- ``-slack_threshold`` specifies slack value below which timing paths need to be analyzed for restructuring
- ``-depth_threshold`` specifies the path depth above which a timing path would be considered for restructuring
- ``tielo_pin`` specifies tie cell pin which can drive constant zero. Format is lib/cell/pin
- ``tiehi_pin`` specifies tie cell pin which can drive constant one. Format is lib/cell/pin
 
### Authors
Sanjiv Mathur
Ahmad El Rouby

## License ##
* [BSD 3-clause License](LICENSE) 


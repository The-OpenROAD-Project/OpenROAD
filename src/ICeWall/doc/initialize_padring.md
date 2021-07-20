# initialize_padring
## Synopsis
```
  % initialize_padring [<signal_mapping_file>]
```
## Description
Generate the padring placement based on the information supplied about the padcells, their locations bondpads or bumps and library cells. If the footprint has been specified without signal mapping, then signal mapping can be done at this stage using the optional signal_mapping_file argument
## Example
```
initialize_padring soc_bsg_black_parrot_nangate45/soc_bsg_black_parrot.sigmap
```

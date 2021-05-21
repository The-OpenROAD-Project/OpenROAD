## ICeWall init_footprint
### Synopsys
```
  % ICeWall init_footprint [<signal_mapping_file>]
```
### Description
Generate the padring placement based on the information supplied about the padcells, their locations bondpads or bumps and library cells. If the footprint has been specified without signal mapping, then signal mapping can be done at this stage using the optional signal_mapping_file argument
### Example
```
ICeWall load_footprint soc_bsg_black_parrot_nangate45/bsg_black_parrot.package.strategy
```

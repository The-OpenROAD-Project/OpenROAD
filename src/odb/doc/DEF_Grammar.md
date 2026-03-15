# DEF Grammar (Railroad Diagram)

The DEF (Design Exchange Format) grammar defines the syntax for design
description files used by OpenROAD's ODB module. The grammar is implemented
as a Bison parser in [`def.y`](../src/def/def/def.y).

## Railroad Diagrams

Railroad diagrams (syntax diagrams) give a visual representation of each
grammar rule. The SVGs below are auto-generated from `def.y` by
[`generate_railroad_diagrams.py`](generate_railroad_diagrams.py) and
regenerated automatically via CI whenever `def.y` changes.

### `def_file`
![def_file](img/def_def_file.svg)

### `rule`
![rule](img/def_rule.svg)

### `design_section`
![design_section](img/def_design_section.svg)

### `comps_section`
![comps_section](img/def_comps_section.svg)

### `nets_section`
![nets_section](img/def_nets_section.svg)

### `snets_section`
![snets_section](img/def_snets_section.svg)

### `pin_section`
![pin_section](img/def_pin_section.svg)

To regenerate the diagrams after editing `def.y`:

```bash
pip install railroad-diagrams
python3 src/odb/doc/generate_railroad_diagrams.py def
```

## Top-Level Grammar Structure

```
def_file ::= version_stmt case_sens_stmt error? rule* end_design

rule ::= design_section | assertions_section | blockage_section
       | comps_section | constraint_section | extension_section
       | fill_section | comps_maskShift_section
       | floorplan_contraints_section [sic] | groups_section
       | iotiming_section | nets_section | nondefaultrule_section
       | partitions_section | pin_props_section | regions_section
       | scanchains_section | slot_section | snets_section
       | styles_section | timingdisables_section | via_section

design_section ::= array_name | bus_bit_chars | canplace
                 | cannotoccupy | design_name | die_area
                 | divider_char | floorplan_name | gcellgrid
                 | history | pin_cap_rule | pin_rule
                 | prop_def_section | row_rule | tech_name
                 | tracks_rule | units

end_design ::= "END" "DESIGN"
```

## Source

- Grammar file: [`def.y`](../src/def/def/def.y) (DEF 5.6, Bison format)
- Full EBNF: [Issue #2488](https://github.com/The-OpenROAD-Project/OpenROAD/issues/2488)

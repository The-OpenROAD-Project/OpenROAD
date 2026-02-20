# DEF Grammar (Railroad Diagram)

The DEF (Design Exchange Format) grammar defines the syntax for design
description files used by OpenROAD's ODB module. The grammar is implemented
as a Bison parser in [`def.y`](../../src/def/def/def.y).

## Viewing the Railroad Diagram

A **railroad diagram** (or syntax diagram) is a visual representation of a
grammar that makes it easy to understand the valid syntax at a glance.

To view the full DEF grammar as an interactive railroad diagram:

1. Open the [Railroad Diagram Generator](https://www.bottlecaps.de/rr/ui)
2. Paste the EBNF grammar from [GitHub issue #2488](https://github.com/The-OpenROAD-Project/OpenROAD/issues/2488) (DEF section)
3. Click the **View Diagram** tab

The EBNF was converted from `def.y` using
[bison-to-w3c](https://www.bottlecaps.de/convert/).

## Top-Level Grammar Structure

Below is a summary of the top-level DEF grammar rules for quick reference:

```
def_file ::= version_stmt case_sens_stmt error? rule* end_design

rule ::= design_section | assertions_section | blockage_section
       | comps_section | constraint_section | extension_section
       | fill_section | comps_maskShift_section
       | floorplan_contraints_section | groups_section
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

- Grammar file: [`def.y`](../../src/def/def/def.y) (DEF 5.6, Bison format)
- Full EBNF: [Issue #2488](https://github.com/The-OpenROAD-Project/OpenROAD/issues/2488)

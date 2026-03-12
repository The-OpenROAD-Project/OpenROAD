# DEF Grammar Railroad Diagrams

The DEF (Design Exchange Format) grammar defines the syntax for the design
description files read and written by OpenROAD's ODB module.  The grammar is
implemented as a Bison parser in
[`src/odb/src/def/def/def.y`](../src/def/def/def.y).

Railroad diagrams (also called syntax diagrams) give an at-a-glance visual
summary of each grammar rule.  The SVGs below are generated directly from
`def.y` by [`generate_railroad_diagrams.py`](generate_railroad_diagrams.py)
and are regenerated automatically via CI whenever `def.y` changes.

## Top-level structure

### `def_file`

![def_file](./images/def/def_file.svg)

### `rule`

![rule](./images/def/rule.svg)

## Design attributes

### `design_section`

![design_section](./images/def/design_section.svg)

## Components

### `comps_section`

![comps_section](./images/def/comps_section.svg)

## Nets

### `nets_section`

![nets_section](./images/def/nets_section.svg)

### `snets_section`

![snets_section](./images/def/snets_section.svg)

## Blockages

### `blockage_section`

![blockage_section](./images/def/blockage_section.svg)

## Pin properties

### `pin_props_section`

![pin_props_section](./images/def/pin_props_section.svg)

## Vias

### `via_section`

![via_section](./images/def/via_section.svg)

## Regenerating the diagrams

After editing `def.y`, re-run:

```shell
python3 src/odb/doc/generate_railroad_diagrams.py def
```

Java 11 or later must be on `PATH`.  On first run the script downloads
`ebnf-convert.war` and `rr.war` from Maven Central into
`src/odb/doc/tools/` (those JARs are not committed to the repository).

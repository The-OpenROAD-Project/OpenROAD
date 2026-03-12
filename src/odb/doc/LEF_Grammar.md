# LEF Grammar Railroad Diagrams

The LEF (Library Exchange Format) grammar defines the syntax for the library
description files read and written by OpenROAD's ODB module.  The grammar is
implemented as a Bison parser in
[`src/odb/src/lef/lef/lef.y`](../src/lef/lef/lef.y).

Railroad diagrams (also called syntax diagrams) give an at-a-glance visual
summary of each grammar rule.  The SVGs below are generated directly from
`lef.y` by [`generate_railroad_diagrams.py`](generate_railroad_diagrams.py)
and are regenerated automatically via CI whenever `lef.y` changes.

## Top-level structure

### `lef_file`

![lef_file](./images/lef/lef_file.svg)

### `rule`

![rule](./images/lef/rule.svg)

## Layer rules

### `layer_rule`

![layer_rule](./images/lef/layer_rule.svg)

## Macro

### `macro`

![macro](./images/lef/macro.svg)

## Via

### `via`

![via](./images/lef/via.svg)

## Site

### `site`

![site](./images/lef/site.svg)

## Regenerating the diagrams

After editing `lef.y`, re-run:

```shell
python3 src/odb/doc/generate_railroad_diagrams.py lef
```

Java 11 or later must be on `PATH`.  On first run the script downloads
`ebnf-convert.war` and `rr.war` from Maven Central into
`src/odb/doc/tools/` (those JARs are not committed to the repository).

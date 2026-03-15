# LEF Grammar (Railroad Diagram)

The LEF (Library Exchange Format) grammar defines the syntax for library
description files used by OpenROAD's ODB module. The grammar is implemented
as a Bison parser in [`lef.y`](../src/lef/lef/lef.y).

## Railroad Diagrams

Railroad diagrams (syntax diagrams) give a visual representation of each
grammar rule. The SVGs below are auto-generated from `lef.y` by
[`generate_railroad_diagrams.py`](generate_railroad_diagrams.py) and
regenerated automatically via CI whenever `lef.y` changes.

### `lef_file`
![lef_file](img/lef_lef_file.svg)

### `rule`
![rule](img/lef_rule.svg)

### `layer_rule`
![layer_rule](img/lef_layer_rule.svg)

### `macro`
![macro](img/lef_macro.svg)

### `via`
![via](img/lef_via.svg)

### `site`
![site](img/lef_site.svg)

To regenerate the diagrams after editing `lef.y`:

```bash
pip install railroad-diagrams
python3 src/odb/doc/generate_railroad_diagrams.py lef
```

## Top-Level Grammar Structure

Below is a summary of the top-level LEF grammar rules for quick reference:

```
lef_file ::= error? rule* extension_opt end_library

rule ::= version | busbitchars | case_sensitivity | units_section
       | layer_rule | via | viarule | viarule_generate | dividerchar
       | wireextension | msg_statement | spacing_rule | dielectric
       | minfeature | irdrop | site | macro | array | def_statement
       | nondefault_rule | prop_def_section | universalnoisemargin
       | edgeratethreshold1 | edgeratescalefactor | edgeratethreshold2
       | noisetable | correctiontable | input_antenna | output_antenna
       | inout_antenna | antenna_input | antenna_inout | antenna_output
       | manufacturing | fixedmask | useminspacing | clearancemeasure
       | maxstack_via | create_file_statement

end_library ::= ( "END" "LIBRARY" )?
```

## Source

- Grammar file: [`lef.y`](../src/lef/lef/lef.y) (LEF 5.6, Bison format)
- Full EBNF: [Issue #2488](https://github.com/The-OpenROAD-Project/OpenROAD/issues/2488)

# LEF Grammar (Railroad Diagram)

The LEF (Library Exchange Format) grammar defines the syntax for library
description files used by OpenROAD's ODB module. The grammar is implemented
as a Bison parser in [`lef.y`](../../src/lef/lef/lef.y).

## Viewing the Railroad Diagram

A **railroad diagram** (or syntax diagram) is a visual representation of a
grammar that makes it easy to understand the valid syntax at a glance.

To view the full LEF grammar as an interactive railroad diagram:

1. Open the [Railroad Diagram Generator](https://www.bottlecaps.de/rr/ui)
2. Paste the EBNF grammar from [GitHub issue #2488](https://github.com/The-OpenROAD-Project/OpenROAD/issues/2488) (LEF section)
3. Click the **View Diagram** tab

The EBNF was converted from `lef.y` using
[bison-to-w3c](https://www.bottlecaps.de/convert/).

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

- Grammar file: [`lef.y`](../../src/lef/lef/lef.y) (LEF 5.6, Bison format)
- Full EBNF: [Issue #2488](https://github.com/The-OpenROAD-Project/OpenROAD/issues/2488)

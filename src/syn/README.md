# Synthesis

OpenROAD's integrated synthesis tool can elaborate SystemVerilog designs and map them to technology.

## Commands

### Elaborate

The `sv_elaborate` command reads SystemVerilog sources, assembles the design hierarchy, and translates the design to a word-level netlist which can be subsequently synthesized using the `synthesize` command. This command accepts [standard slang command-line options](https://sv-lang.com/command-line-ref.html).

```tcl
sv_elaborate <slang options>
```

### Synthesize

The `synthesize` command synthesizes a previously elaborated design, i.e. it implements the design using available standard cells. The mapped gate-level netlist is loaded into OpenROAD's design database (ODB) for other tools to operate on.

```tcl
synthesize
    [-reduce_name_loss]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-reduce_name_loss` | Disable any optimization steps which would cause mass loss of intermediate signal names. This option trades away quality of results for interpretability. |

## Limitations

The integrated synthesis tool does not support mapping to latches, always flattens the design, and loses the names of instantiated macros.

## Authors

The original `syn` was developed and contributed by Martin Povišer of Precision Innovations Inc. Design of the intermediate representation used by syn internally derives from the [prjunnamed project](https://prjunnamed.org/): `Copyright (C) Project Unnamed contributors`; distributed under the terms of the BSD-0 license.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.

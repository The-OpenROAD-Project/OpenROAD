# OpenPhySyn Hello Transform

A simple template for building Physical Synthesis transforms for [OpenPhySyn](https://github.com/The-OpenROAD-Project/OpenPhySyn) physical synthesis tool.

## Building

Build by making a build directory (i.e. `build/`), run `cmake ..` in that directory, and then use `make` to build the desired target.

Example:

```bash
> mkdir build && cd build
> cmake .. -DPSN_HOME=<OpenPhySyn Source Code Path> \
> -DOPENDB_HOME=<OpenDB Source Code Directory> \
> -DOPENSTA_HOME=<OpenSTA Source Code Directory>
> make
> make install # Or sudo make install
```

## Usage

```bash
> ./Psn
> import_lef <lef file>
> import_def <def file>
> # Add random wire
> transform hello_transform net1
> export_def out.def
```
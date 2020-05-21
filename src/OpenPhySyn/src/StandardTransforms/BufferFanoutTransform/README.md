# OpenPhySyn Buffer Fan-Out Transform

A simple buffering transform using [OpenPhySyn](https://github.com/The-OpenROAD-Project/OpenPhySyn) physical synthesis tool.

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
> # Run simple buffering algorithm for cell with max-fanout 2
> transform buffer_fanout 2 BUF_X1
> export_def out.def
```

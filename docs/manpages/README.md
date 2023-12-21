# Man pages implementation

## Prototype 1 (pandoc)

- To install pandoc, refer to this [link](https://github.com/jgm/pandoc/blob/main/INSTALL.md). `apt-get` *should* just work for Ubuntu. 
- Use pandoc to convert markdown to roff format.  
- Ensure that all the source files are in `src` folder before runnig. 
```
make clean && make all
```

- Setup environment variables as follows. This is to avoid overwriting the system default for `MANPATH`.
```
echo "MANDATORY_MANPATH $(pwd)" >> ~/.manpath
```

- Afterwards, you can read your manpages using:
```
man template
```

### What about inside OpenROAD?

- To run `man` commands inside OpenROAD, you can either use the Linux `man` binary:
```tcl
# create a man wrapper
source scripts/main.tcl
man openroad
```

- Or the `Tcl` script that outputs raw text.
```tcl
source scripts/Utl.tcl
# you will be prompted to enter the RELATIVE path to cat folders.
man openroad
```

## Prototype 2 (git-pages)

- TODO, they use another word processor that seems more versatile called AsciiDoc.
- Reference: https://github.com/git/git/tree/master/Documentation 

## TODO
- pandoc not the most perfect, as pandoc does not seem to do the indentation perfectly as in other manpages. for the options section
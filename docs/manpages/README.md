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

## Prototype 2 (git-pages)

- TOdo. 

## TODO
- pandoc not the most perfect, as pandoc does not seem to do the indentation perfectly as in other manpages. for the options section
- Need to add to `MANPATH`: https://unix.stackexchange.com/questions/344603/how-to-append-to-manpath

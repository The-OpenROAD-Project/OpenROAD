# OpenROAD Documentation

This `docs/` hierarchy houses code and raw files to 
build the on-line documentation (using Sphinx) and 
manual pages using (using Pandoc)

This on-line documentation is available at [https://openroad.readthedocs.io/en/latest/](https://openroad.readthedocs.io/en/latest/).

## Prerequisites

- To install pandoc, refer to this [link](https://github.com/jgm/pandoc/blob/main/INSTALL.md). `apt-get` *should* just work for Ubuntu. 
- To install sphinx requirements, **create a virtual environment (e.g. conda/virtualenv)** and then run `pip install -r requirements.txt`.

### Build instructions for Pandoc manpages

The `-j16` command is optional for speeding up the manpage compilation process by using multiple jobs
based on the number of cores in your system.

```shell
make clean

# Note this step is important as it regenerates the documentation using latest sources.
make preprocess && make all -j16
```

#### To view manpages

- To run `man` commands inside OpenROAD, you can either use the Linux `man` binary:
```tcl
# create a man wrapper
source man/scripts/main.tcl
man openroad
```

- Or just within OpenROAD itself.
```tcl
# you will be prompted to enter the RELATIVE path to cat folders which is optional.
man openroad
```

### Build instructions for Sphinx docs

#### HTML docs

``` shell
make html
```

#### Check for broken links

``` shell
make checklinks
```

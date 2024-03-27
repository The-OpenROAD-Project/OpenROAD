# OpenROAD Documentation

This documentation is available at [https://openroad.readthedocs.io/en/latest/](https://openroad.readthedocs.io/en/latest/)

## Build locally

### Requires:
- Python 3.x
- Pip
- `virtualenv`
- `doxygen`

### Install prerequisites:

You may install Doxygen from this [link](https://www.doxygen.nl/download.html).
Most methods of installation are fine, just ensure that `doxygen` is in $PATH. 
This is needed for Doxygen compilation. 

To install Python packages:

``` shell
virtualenv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### Build:

``` shell
make html
```

### Check for broken links:

``` shell
make checklinks
```

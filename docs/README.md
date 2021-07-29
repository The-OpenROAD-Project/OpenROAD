# OpenROAD Documentation

This documentation is available at [https://openroad.readthedocs.io/en/latest/](https://openroad.readthedocs.io/en/latest/)

## Build locally

### Requires:
- Python 3.x
- Pip
- `virtualenv`

### Install pre-requisites

``` shell
virtualenv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### Build

``` shell
make html
```

### Check for broken links

``` shell
make checklinks
```

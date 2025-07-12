# OpenROAD IP Checker

Utility for checking supported IP blocks in the OpenROAD project.

## Features
- Check if an IP is supported
- List all supported IPs
- JSON output option
- Colorized terminal output

## Installation
```bash
git clone https://github.com/yourusername/openroad-ip-checker.git
cd openroad-ip-checker
```

## Dependencies
- Python 3.7+

## Usage
```bash
# Check an IP
./ip_checker.py <ip_name>

# List all supported IPs
./ip_checker.py --list

# Use custom IP file
./ip_checker.py --file custom_ips.txt sha256

# JSON output
./ip_checker.py --json aes
```

## Examples
```bash
$ ./ip_checker.py aes
Supported

$ ./ip_checker.py unknown_ip
Unsupported

$ ./ip_checker.py --list
Supported IPs:
- aes
- gcd
- jpeg_encoder
- riscv
- sha256
```

## Testing
```bash
pip install -r requirements.txt
pytest --cov=ip_checker tests/
```

## CI Status
[![CI](https://github.com/yourusername/openroad-ip-checker/actions/workflows/ci.yml/badge.svg)](https://github.com/yourusername/openroad-ip-checker/actions)
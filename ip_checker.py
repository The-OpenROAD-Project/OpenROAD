#!/usr/bin/env python3
"""
OpenROAD IP Checker Utility
Check if an IP block is supported by OpenROAD
"""

import argparse
import sys
import os
import json
from pathlib import Path
from datetime import datetime

class IPChecker:
    """Core IP checking functionality"""
    
    def __init__(self, file_path='supported_ips.txt'):
        self.file_path = Path(file_path)
        self.supported_ips = self._load_ips()
    
    def _load_ips(self) -> set:
        """Load IPs from file with error handling"""
        try:
            content = self.file_path.read_text(encoding='utf-8')
            ips = set()
            for line in content.splitlines():
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                ips.add(line.lower())
            return ips
        except FileNotFoundError:
            sys.stderr.write(f"Error: IP file '{self.file_path}' not found\n")
            sys.exit(1)
    
    def is_supported(self, ip_name: str) -> bool:
        """Check if IP is supported (case-insensitive)"""
        return ip_name.lower() in self.supported_ips
    
    def list_ips(self) -> list:
        """Get sorted list of supported IPs"""
        return sorted(self.supported_ips)

def colorize(text: str, color_code: str) -> str:
    """Add ANSI color codes if running in terminal"""
    return f"\033[{color_code}m{text}\033[0m" if sys.stdout.isatty() else text

def main():
    """Command-line interface handler"""
    parser = argparse.ArgumentParser(
        description='OpenROAD IP Support Checker',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        'ip', 
        nargs='?', 
        help='IP block name to verify'
    )
    parser.add_argument(
        '--list', 
        action='store_true',
        help='List all supported IPs'
    )
    parser.add_argument(
        '--file',
        default='supported_ips.txt',
        help='Path to supported IPs file'
    )
    parser.add_argument(
        '--json',
        action='store_true',
        help='Output results in JSON format'
    )
    
    args = parser.parse_args()
    
    # Handle --list command
    if args.list:
        checker = IPChecker(args.file)
        ips = checker.list_ips()
        
        if args.json:
            print(json.dumps({
                "supported_ips": ips,
                "timestamp": datetime.utcnow().isoformat() + "Z"
            }))
        else:
            print("Supported IPs:")
            for ip in ips:
                print(f"- {ip}")
        return
    
    # Handle missing IP argument
    if not args.ip:
        parser.error("Please provide an IP name or use --list")
    
    # Check single IP
    checker = IPChecker(args.file)
    supported = checker.is_supported(args.ip)
    
    if args.json:
        print(json.dumps({
            "ip": args.ip,
            "supported": supported,
            "timestamp": datetime.utcnow().isoformat() + "Z"
        }))
    else:
        status = "Supported" if supported else "Unsupported"
        if sys.stdout.isatty():
            status = colorize(status, "32" if supported else "31")  # Green/Red
        print(status)

if __name__ == "__main__":
    main()
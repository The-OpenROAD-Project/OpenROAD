import os
import sys
import json
import pytest
from pathlib import Path
from datetime import datetime
from ip_checker import IPChecker

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

TEST_IPS = Path(__file__).parent / "test_ips.txt"

@pytest.fixture
def test_ips(tmp_path):
    """Create test IP file"""
    ip_file = tmp_path / "test_ips.txt"
    ip_file.write_text("""
    # Test IPs
    aes
    gcd
    sha256
    
    riscv
    """)
    return ip_file

def test_load_ips(test_ips):
    """Test IP loading from file"""
    checker = IPChecker(test_ips)
    assert checker.supported_ips == {'aes', 'gcd', 'sha256', 'riscv'}

def test_case_insensitivity(test_ips):
    """Test case-insensitive matching"""
    checker = IPChecker(test_ips)
    assert checker.is_supported('AES')
    assert checker.is_supported('gCd')
    assert not checker.is_supported('unknown')

def test_missing_file():
    """Test missing file handling"""
    with pytest.raises(SystemExit) as exc:
        IPChecker('non_existent_file.txt')
    assert exc.value.code == 1

def test_list_ips(test_ips, capsys):
    """Test --list functionality"""
    checker = IPChecker(test_ips)
    assert checker.list_ips() == ['aes', 'gcd', 'riscv', 'sha256']

def test_json_output(test_ips, capsys):
    """Test JSON output format"""
    checker = IPChecker(test_ips)
    checker.is_supported('aes')  # Implementation detail for coverage
    
    # Mock command line arguments
    sys.argv = ['ip_checker.py', '--json', '--file', str(test_ips), 'aes']
    import ip_checker
    ip_checker.main()
    
    output = json.loads(capsys.readouterr().out)
    assert output['ip'] == 'aes'
    assert output['supported'] is True
    assert 'timestamp' in output
# Security Policy

## Reporting a Vulnerability

If you discover a security vulnerability in OpenROAD, please do NOT open a public GitHub issue.
Instead, send details to the OpenROAD maintainers through the project's communication channels.

### What we need from you

- A clear description of the vulnerability
- Steps to reproduce (or proof-of-concept code)
- Affected versions and components
- Any suggested fix (optional)

### What to expect

- Acknowledgment within 5 business days
- A timeline for triage and fix
- Credit in release notes (unless you prefer anonymity)

## Supported Versions

| Version | Supported |
|---------|-----------|
| master  | ✅ Active development |
| latest release | ✅ Security patches |
| older releases | ❌ No support |

## Disclosure Policy

We follow coordinated disclosure:

1. Fix prepared internally or via PR
2. Embargo period for users to patch
3. Public disclosure with CVE assignment

## Scope

Security issues include, but are not limited to:

- Remote code execution in any interface or component
- Supply chain integrity (e.g., compromised dependencies)
- Unauthorized access to design data
- Memory corruption vulnerabilities
- Denial of service via crafted input

Non-security bugs should be filed as regular GitHub issues.

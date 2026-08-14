import traceback

import utl

try:
    utl.error(utl.CTS, 99, "Arbitrary CTS error message")
except RuntimeError:
    traceback.print_exc()

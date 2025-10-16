import utl

try:
    utl.error(utl.CTS, 99, "Arbitrary CTS error message")
except Exception as inst:
    print(inst.args[0])

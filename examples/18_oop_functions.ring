/*
 * Example 18: OOP - Function Calling
 * API: FFI (cFunc, funcPtr, varFunc, invoke, varcall, sym, string)
 */
load "cffi.ring"

oFFI = new FFI("libc.so.6")

# cFunc + invoke (OOP wrapper)
oStrlen = oFFI.cFunc("strlen", "int", ["ptr"])
pStr = oFFI.string("Hello OOP!")
? "strlen: " + oFFI.invoke(oStrlen, [pStr])

# funcPtr via sym
pSym = oFFI.sym("atoi")
oAtoi = oFFI.funcPtr(pSym, "int", ["ptr"])
? "atoi: " + oFFI.invoke(oAtoi, [oFFI.string("12345")])

# varFunc + varcall
oPrintf = oFFI.varFunc("printf", "int", 1, ["ptr"])
oFFI.varcall(oPrintf, [oFFI.string("OOP printf: %d + %d = %d\n"), 10, 20, 30])

/*
 * Example 12: Variadic Functions (C API)
 * API: cffi_varfunc, cffi_varcall
 */
load "cffi.ring"

pLib = cffi_load("libc.so.6")

oPrintf = cffi_varfunc(pLib, "printf", "int", 1, ["ptr"])
pFmt = cffi_string("Value: %d, Sum: %d\n")
cffi_varcall(oPrintf, pFmt, 42, 100)

/*
 * Example 05: Strings (C API)
 * API: cffi_string, cffi_tostring
 */
load "cffi.ring"

pLib = cffi_load("libc.so.6")

pStr = cffi_string("Hello from Ring!")
? "C string: " + cffi_tostring(pStr)

oFunc = cffi_func(pLib, "strlen", "int", ["ptr"])
? "Length: " + cffi_invoke(oFunc, pStr)

pDest = cffi_new("char", 64)
oStrcpy = cffi_func(pLib, "strcpy", "ptr", ["ptr", "ptr"])
cffi_invoke(oStrcpy, pDest, pStr)
? "Copied: " + cffi_tostring(pDest)

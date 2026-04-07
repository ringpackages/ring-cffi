/*
 * Example 06: Symbol Resolution (C API)
 * API: cffi_sym
 */
load "cffi.ring"

pLib = cffi_load("libc.so.6")

pSym = cffi_sym(pLib, "strlen")
? "strlen symbol: " + (not cffi_isnull(pSym))

pBad = cffi_sym(pLib, "nonexistent_xyz")
? "nonexistent is null: " + cffi_isnull(pBad)

oFunc = cffi_funcptr(pSym, "int", ["ptr"])
? "via funcptr: " + cffi_invoke(oFunc, cffi_string("test"))

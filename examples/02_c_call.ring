/*
 * Example 02: Call C Functions (C API)
 * API: cffi_func, cffi_invoke, cffi_string, cffi_tostring
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

oFunc = cffi_func(pLib, "strlen", "int", ["ptr"])
pStr = cffi_string("Hello, World!")
? "strlen: " + cffi_invoke(oFunc, pStr)

oCmp = cffi_func(pLib, "strcmp", "int", ["ptr", "ptr"])
? "strcmp: " + cffi_invoke(oCmp, cffi_string("abc"), cffi_string("abc"))

oAtoi = cffi_func(pLib, "atoi", "int", ["ptr"])
? "atoi: " + cffi_invoke(oAtoi, cffi_string("42"))

func getLibcPath
    if isWindows()
        return "msvcrt.dll"
    but isFreeBSD()
        return "libc.so.7"
    but isMacOSX()
        return "libSystem.B.dylib"
    else
        return "libc.so.6"
    ok
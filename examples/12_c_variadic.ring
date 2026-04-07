/*
 * Example 12: Variadic Functions (C API)
 * API: cffi_varfunc, cffi_varcall
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

oPrintf = cffi_varfunc(pLib, "printf", "int", 1, ["ptr"])
pFmt = cffi_string("Value: %d, Sum: %d\n")
cffi_varcall(oPrintf, pFmt, 42, 100)

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
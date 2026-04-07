/*
 * Example 01: Load Library (C API)
 * API: cffi_load, cffi_isnull
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())
? "Library loaded: " + (not cffi_isnull(pLib))

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
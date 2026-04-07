/*
 * Example 13: C Definition Parser (C API)
 * API: cffi_cdef
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

cDef = "
    struct Point {
        int x;
        int y;
    };
"

nParsed = cffi_cdef(pLib, cDef)
? "Parsed " + nParsed + " definitions"

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